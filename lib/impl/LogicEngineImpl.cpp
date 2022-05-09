//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LogicEngineImpl.h"

#include "ramses-framework-api/RamsesVersion.h"
#include "ramses-logic/LogicNode.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/TimerNode.h"
#include "ramses-logic/AnimationNodeConfig.h"

#include "impl/LogicNodeImpl.h"
#include "impl/LoggerImpl.h"
#include "impl/LuaModuleImpl.h"
#include "impl/LuaConfigImpl.h"
#include "impl/SaveFileConfigImpl.h"
#include "impl/LogicEngineReportImpl.h"

#include "internals/FileUtils.h"
#include "internals/TypeUtils.h"
#include "internals/RamsesObjectResolver.h"
#include "internals/ApiObjects.h"

#include "generated/LogicEngineGen.h"
#include "ramses-logic-build-config.h"

#include "fmt/format.h"

#include <string>
#include <fstream>
#include <streambuf>

namespace rlogic::internal
{
    LogicEngineImpl::LogicEngineImpl()
        : m_apiObjects(std::make_unique<ApiObjects>())
    {
    }

    LogicEngineImpl::~LogicEngineImpl() noexcept = default;

    LuaScript* LogicEngineImpl::createLuaScript(std::string_view source, const LuaConfigImpl& config, std::string_view scriptName)
    {
        m_errors.clear();
        return m_apiObjects->createLuaScript(source, config, scriptName, m_errors);
    }

    LuaInterface* LogicEngineImpl::createLuaInterface(std::string_view source, std::string_view interfaceName)
    {
        m_errors.clear();
        return m_apiObjects->createLuaInterface(source, interfaceName, m_errors);
    }

    LuaModule* LogicEngineImpl::createLuaModule(std::string_view source, const LuaConfigImpl& config, std::string_view moduleName)
    {
        m_errors.clear();
        return m_apiObjects->createLuaModule(source, config, moduleName, m_errors);
    }

    bool LogicEngineImpl::extractLuaDependencies(std::string_view source, const std::function<void(const std::string&)>& callbackFunc)
    {
        m_errors.clear();
        const std::optional<std::vector<std::string>> extractedDependencies = LuaCompilationUtils::ExtractModuleDependencies(source, m_errors);
        if (!extractedDependencies)
            return false;

        for (const auto& dep : *extractedDependencies)
            callbackFunc(dep);

        return true;
    }

    RamsesNodeBinding* LogicEngineImpl::createRamsesNodeBinding(ramses::Node& ramsesNode, ERotationType rotationType, std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects->createRamsesNodeBinding(ramsesNode, rotationType,  name);
    }

    RamsesAppearanceBinding* LogicEngineImpl::createRamsesAppearanceBinding(ramses::Appearance& ramsesAppearance, std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects->createRamsesAppearanceBinding(ramsesAppearance, name);
    }

    RamsesCameraBinding* LogicEngineImpl::createRamsesCameraBinding(ramses::Camera& ramsesCamera, std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects->createRamsesCameraBinding(ramsesCamera, name);
    }

    template <typename T>
    DataArray* LogicEngineImpl::createDataArray(const std::vector<T>& data, std::string_view name)
    {
        static_assert(CanPropertyTypeBeStoredInDataArray(PropertyTypeToEnum<T>::TYPE));
        m_errors.clear();
        if (data.empty())
        {
            m_errors.add(fmt::format("Cannot create DataArray '{}' with empty data.", name), nullptr, EErrorType::IllegalArgument);
            return nullptr;
        }

        return m_apiObjects->createDataArray(data, name);
    }

    rlogic::AnimationNode* LogicEngineImpl::createAnimationNode(const AnimationNodeConfig& config, std::string_view name)
    {
        m_errors.clear();

        auto containsDataArray = [this](const DataArray* da) {
            const auto& dataArrays = m_apiObjects->getApiObjectContainer<DataArray>();
            const auto it = std::find_if(dataArrays.cbegin(), dataArrays.cend(),
                [da](const auto& d) { return d == da; });
            return it != dataArrays.cend();
        };

        if (config.getChannels().empty())
        {
            m_errors.add(fmt::format("Failed to create AnimationNode '{}': must provide at least one channel.", name), nullptr, EErrorType::IllegalArgument);
            return nullptr;
        }

        for (const auto& channel : config.getChannels())
        {
            if (!containsDataArray(channel.timeStamps) ||
                !containsDataArray(channel.keyframes))
            {
                m_errors.add(fmt::format("Failed to create AnimationNode '{}': timestamps or keyframes were not found in this logic instance.", name), nullptr, EErrorType::IllegalArgument);
                return nullptr;
            }

            if ((channel.tangentsIn && !containsDataArray(channel.tangentsIn)) ||
                (channel.tangentsOut && !containsDataArray(channel.tangentsOut)))
            {
                m_errors.add(fmt::format("Failed to create AnimationNode '{}': tangents were not found in this logic instance.", name), nullptr, EErrorType::IllegalArgument);
                return nullptr;
            }
        }

        return m_apiObjects->createAnimationNode(*config.m_impl, name);
    }

    TimerNode* LogicEngineImpl::createTimerNode(std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects->createTimerNode(name);
    }

    bool LogicEngineImpl::destroy(LogicObject& object)
    {
        m_errors.clear();
        return m_apiObjects->destroy(object, m_errors);
    }

    bool LogicEngineImpl::isLinked(const LogicNode& logicNode) const
    {
        return m_apiObjects->getLogicNodeDependencies().isLinked(logicNode.m_impl);
    }

    size_t LogicEngineImpl::activateLinksRecursive(PropertyImpl& output)
    {
        size_t activatedLinks = 0u;

        const auto childCount = output.getChildCount();
        for (size_t i = 0; i < childCount; ++i)
        {
            PropertyImpl& child = *output.getChild(i)->m_impl;

            if (TypeUtils::CanHaveChildren(child.getType()))
            {
                activatedLinks += activateLinksRecursive(child);
            }
            else
            {
                const auto& outgoingLinks = child.getOutgoingLinks();
                for (const auto& outLink : outgoingLinks)
                {
                    PropertyImpl* linkedProp = outLink.property;
                    const bool valueChanged = linkedProp->setValue(child.getValue());
                    if (valueChanged || linkedProp->getPropertySemantics() == EPropertySemantics::AnimationInput)
                    {
                        linkedProp->getLogicNode().setDirty(true);
                        ++activatedLinks;
                    }
                }
            }
        }

        return activatedLinks;
    }

    bool LogicEngineImpl::update()
    {
        m_errors.clear();

        if (m_statisticsEnabled || m_updateReportEnabled)
        {
            m_updateReport.clear();
            m_updateReport.sectionStarted(UpdateReport::ETimingSection::TotalUpdate);
        }
        if (m_updateReportEnabled)
        {
            m_updateReport.sectionStarted(UpdateReport::ETimingSection::TopologySort);
        }

        const std::optional<NodeVector>& sortedNodes = m_apiObjects->getLogicNodeDependencies().getTopologicallySortedNodes();
        if (!sortedNodes)
        {
            m_errors.add("Failed to sort logic nodes based on links between their properties. Create a loop-free link graph before calling update()!", nullptr, EErrorType::ContentStateError);
            return false;
        }

        if (m_updateReportEnabled)
            m_updateReport.sectionFinished(UpdateReport::ETimingSection::TopologySort);

        // force dirty all timer nodes and their dependents so they update their tickers
        setTimerNodesDirty();

        const bool success = updateNodes(*sortedNodes);

        if (m_statisticsEnabled || m_updateReportEnabled)
        {
            m_updateReport.sectionFinished(UpdateReport::ETimingSection::TotalUpdate);
            m_statistics.collect(m_updateReport, sortedNodes->size());
            if (m_statistics.checkUpdateFrameFinished())
                m_statistics.calculateAndLog();
        }

        return success;
    }

    bool LogicEngineImpl::updateNodes(const NodeVector& sortedNodes)
    {
        for (LogicNodeImpl* nodeIter : sortedNodes)
        {
            LogicNodeImpl& node = *nodeIter;

            if (!node.isDirty())
            {
                if (m_updateReportEnabled)
                    m_updateReport.nodeSkippedExecution(node);

                if(m_nodeDirtyMechanismEnabled)
                    continue;
            }

            if (m_updateReportEnabled)
                m_updateReport.nodeExecutionStarted(node);
            if (m_statisticsEnabled)
                m_statistics.nodeExecuted();

            const std::optional<LogicNodeRuntimeError> potentialError = node.update();
            if (potentialError)
            {
                m_errors.add(potentialError->message, m_apiObjects->getApiObject(node), EErrorType::RuntimeError);
                return false;
            }

            Property* outputs = node.getOutputs();
            if (outputs != nullptr)
            {
                const size_t activatedLinks = activateLinksRecursive(*outputs->m_impl);

                if (m_statisticsEnabled || m_updateReportEnabled)
                    m_updateReport.linksActivated(activatedLinks);
            }

            if (m_updateReportEnabled)
                m_updateReport.nodeExecutionFinished();

            node.setDirty(false);
        }

        return true;
    }

    void LogicEngineImpl::setTimerNodesDirty()
    {
        for (TimerNode* timerNode : m_apiObjects->getApiObjectContainer<TimerNode>())
        {
            // force set timer node itself dirty so it can update its ticker
            timerNode->m_impl.setDirty(true);
        }
    }

    const std::vector<ErrorData>& LogicEngineImpl::getErrors() const
    {
        return m_errors.getErrors();
    }

    const std::vector<WarningData>& LogicEngineImpl::validate() const
    {
        m_validationResults.clear();
        if (m_apiObjects->bindingsDirty())
        {
            m_validationResults.add("Saving logic engine content with manually updated binding values without calling update() will result in those values being lost!", nullptr, EWarningType::UnsafeDataState);
        }

        m_apiObjects->checkAllInterfaceOutputsLinked(m_validationResults);

        return m_validationResults.getWarnings();
    }

    bool LogicEngineImpl::CheckRamsesVersionFromFile(const rlogic_serialization::Version& ramsesVersion)
    {
        // Only major version changes result in file incompatibilities
        return static_cast<int>(ramsesVersion.v_major()) == ramses::GetRamsesVersion().major;
    }

    bool LogicEngineImpl::loadFromBuffer(const void* rawBuffer, size_t bufferSize, ramses::Scene* scene, bool enableMemoryVerification)
    {
        return loadFromByteData(rawBuffer, bufferSize, scene, enableMemoryVerification, fmt::format("data buffer '{}' (size: {})", rawBuffer, bufferSize));
    }

    bool LogicEngineImpl::loadFromFile(std::string_view filename, ramses::Scene* scene, bool enableMemoryVerification)
    {
        std::optional<std::vector<char>> maybeBytesFromFile = FileUtils::LoadBinary(std::string(filename));
        if (!maybeBytesFromFile)
        {
            m_errors.add(fmt::format("Failed to load file '{}'", filename), nullptr, EErrorType::BinaryDataAccessError);
            return false;
        }

        const size_t fileSize = (*maybeBytesFromFile).size();
        return loadFromByteData((*maybeBytesFromFile).data(), fileSize, scene, enableMemoryVerification, fmt::format("file '{}' (size: {})", filename, fileSize));
    }

    bool LogicEngineImpl::checkFileIdentifierBytes(const std::string& dataSourceDescription, const std::string& fileIdBytes)
    {
        const std::string expected(rlogic_serialization::LogicEngineIdentifier());
        if (expected.substr(0, 2) != fileIdBytes.substr(0, 2))
        {
            m_errors.add(fmt::format("{}: Tried loading a binary data which doesn't store Ramses Logic content! Expected file bytes 4-5 to be '{}', but found '{}' instead",
                dataSourceDescription,
                expected.substr(0, 2),
                fileIdBytes.substr(0, 2)
            ), nullptr, EErrorType::BinaryDataAccessError);
            return false;
        }

        if (expected.substr(2, 2) != fileIdBytes.substr(2, 2))
        {
            m_errors.add(fmt::format("{}: Version mismatch while loading binary data! Expected version '{}', but found '{}'",
                dataSourceDescription,
                expected.substr(2, 2),
                fileIdBytes.substr(2, 2)
            ),nullptr, EErrorType::BinaryVersionMismatch);
            return false;
        }

        return true;
    }

    // TODO Violin consider handling errors gracefully, e.g. don't change state when error occurs
    // Idea: collect data, and only move() in the end when everything was loaded correctly
    bool LogicEngineImpl::loadFromByteData(const void* byteData, size_t byteSize, ramses::Scene* scene, bool enableMemoryVerification, const std::string& dataSourceDescription)
    {
        m_errors.clear();

        if (byteSize < 8)
        {
            m_errors.add(fmt::format("{} contains corrupted data! Data should be at least 8 bytes", dataSourceDescription), nullptr, EErrorType::BinaryDataAccessError);
            return false;
        }

        auto* char8Data(static_cast<const char*>(byteData));
        // file identifier bytes are always placed at bytes 4-7 in the buffer
        const std::string fileIdBytes(&char8Data[4], 4); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) No better options
        if (!checkFileIdentifierBytes(dataSourceDescription, fileIdBytes))
        {
            return false;
        }

        auto* uint8Data(static_cast<const uint8_t*>(byteData));
        if (enableMemoryVerification)
        {
            flatbuffers::Verifier bufferVerifier(uint8Data, byteSize);
            const bool bufferOK = rlogic_serialization::VerifyLogicEngineBuffer(bufferVerifier);

            if (!bufferOK)
            {
                m_errors.add(fmt::format("{} contains corrupted data!", dataSourceDescription), nullptr, EErrorType::BinaryDataAccessError);
                return false;
            }
        }

        const auto* logicEngine = rlogic_serialization::GetLogicEngine(byteData);

        if (nullptr == logicEngine)
        {
            m_errors.add(fmt::format("{} doesn't contain logic engine data with readable version specifiers", dataSourceDescription), nullptr, EErrorType::BinaryVersionMismatch);
            return false;
        }

        const auto& ramsesVersion = *logicEngine->ramsesVersion();
        const auto& rlogicVersion = *logicEngine->rlogicVersion();

        LOG_INFO("Loading logic engine content from '{}' which was exported with Ramses {} and Logic Engine {}", dataSourceDescription, ramsesVersion.v_string()->string_view(), rlogicVersion.v_string()->string_view());

        if (!CheckRamsesVersionFromFile(ramsesVersion))
        {
            m_errors.add(fmt::format("Version mismatch while loading {}! Expected Ramses version {}.x.x but found {}",
                dataSourceDescription, ramses::GetRamsesVersion().major,
                ramsesVersion.v_string()->string_view()), nullptr, EErrorType::BinaryVersionMismatch);
            return false;
        }

        if (nullptr == logicEngine->apiObjects())
        {
            m_errors.add(fmt::format("Fatal error while loading {}: doesn't contain API objects!", dataSourceDescription), nullptr, EErrorType::BinaryVersionMismatch);
            return false;
        }

        if (logicEngine->assetMetadata())
        {
            LogAssetMetadata(*logicEngine->assetMetadata());
        }

        RamsesObjectResolver ramsesResolver(m_errors, scene);

        std::unique_ptr<ApiObjects> deserializedObjects = ApiObjects::Deserialize(*logicEngine->apiObjects(), ramsesResolver, dataSourceDescription, m_errors);

        if (!deserializedObjects)
        {
            return false;
        }

        // No errors -> move data into member
        m_apiObjects = std::move(deserializedObjects);

        return true;
    }

    bool LogicEngineImpl::saveToFile(std::string_view filename, const SaveFileConfigImpl& config)
    {
        m_errors.clear();

        if (!m_apiObjects->checkBindingsReferToSameRamsesScene(m_errors))
        {
            m_errors.add("Can't save a logic engine to file while it has references to more than one Ramses scene!", nullptr, EErrorType::ContentStateError);
            return false;
        }

        // Refuse save() if logic graph has loops
        if (!m_apiObjects->getLogicNodeDependencies().getTopologicallySortedNodes())
        {
            m_errors.add("Failed to sort logic nodes based on links between their properties. Create a loop-free link graph before calling saveToFile()!", nullptr, EErrorType::ContentStateError);
            return false;
        }

        if (config.getValidationEnabled())
        {
            const std::vector<WarningData>& warnings = validate();

            if (!warnings.empty())
            {
                m_errors.add(
                    "Failed to saveToFile() because validation warnings were encountered! "
                    "Refer to the documentation of saveToFile() for details how to address these gracefully.", nullptr, EErrorType::ContentStateError);
                return false;
            }
        }

        flatbuffers::FlatBufferBuilder builder;
        ramses::RamsesVersion ramsesVersion = ramses::GetRamsesVersion();

        const auto ramsesVersionOffset = rlogic_serialization::CreateVersion(builder,
            ramsesVersion.major,
            ramsesVersion.minor,
            ramsesVersion.patch,
            builder.CreateString(ramsesVersion.string));
        builder.Finish(ramsesVersionOffset);

        const auto ramsesLogicVersionOffset = rlogic_serialization::CreateVersion(builder,
            g_PROJECT_VERSION_MAJOR,
            g_PROJECT_VERSION_MINOR,
            g_PROJECT_VERSION_PATCH,
            builder.CreateString(g_PROJECT_VERSION));
        builder.Finish(ramsesLogicVersionOffset);

        const auto exporterVersionOffset = rlogic_serialization::CreateVersion(builder,
            config.getExporterMajorVersion(),
            config.getExporterMinorVersion(),
            config.getExporterPatchVersion(),
            builder.CreateString(""));
        builder.Finish(exporterVersionOffset);

        const auto assetMetadataOffset = rlogic_serialization::CreateMetadata(builder,
            builder.CreateString(config.getMetadataString()),
            exporterVersionOffset,
            config.getExporterFileFormatVersion());
        builder.Finish(assetMetadataOffset);

        const auto logicEngine = rlogic_serialization::CreateLogicEngine(builder,
            ramsesVersionOffset,
            ramsesLogicVersionOffset,
            ApiObjects::Serialize(*m_apiObjects, builder),
            assetMetadataOffset);

        FinishLogicEngineBuffer(builder, logicEngine);

        if (!FileUtils::SaveBinary(std::string(filename), builder.GetBufferPointer(), builder.GetSize()))
        {
            m_errors.add(fmt::format("Failed to save content to path '{}'!", filename), nullptr, EErrorType::BinaryDataAccessError);
            return false;
        }

        LOG_INFO("Saved logic engine to file: '{}'.", filename);

        return true;
    }

    void LogicEngineImpl::LogAssetMetadata(const rlogic_serialization::Metadata& assetMetadata)
    {
        const std::string_view metadataString = assetMetadata.metadataString() ? assetMetadata.metadataString()->string_view() : "none";
        LOG_INFO("Logic Engine content metadata: '{}'", metadataString);
        const std::string exporterVersion = assetMetadata.exporterVersion() ?
            fmt::format("{}.{}.{} (file format version {})",
                assetMetadata.exporterVersion()->v_major(),
                assetMetadata.exporterVersion()->v_minor(),
                assetMetadata.exporterVersion()->v_patch(),
                assetMetadata.exporterFileVersion()) : "undefined";
        LOG_INFO("Exporter version: {}", exporterVersion);
    }

    bool LogicEngineImpl::link(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_apiObjects->getLogicNodeDependencies().link(*sourceProperty.m_impl, *targetProperty.m_impl, false, m_errors);
    }

    bool LogicEngineImpl::linkWeak(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_apiObjects->getLogicNodeDependencies().link(*sourceProperty.m_impl, *targetProperty.m_impl, true, m_errors);
    }

    bool LogicEngineImpl::unlink(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_apiObjects->getLogicNodeDependencies().unlink(*sourceProperty.m_impl, *targetProperty.m_impl, m_errors);
    }

    ApiObjects& LogicEngineImpl::getApiObjects()
    {
        return *m_apiObjects;
    }

    void LogicEngineImpl::disableTrackingDirtyNodes()
    {
        m_nodeDirtyMechanismEnabled = false;
    }

    void LogicEngineImpl::enableUpdateReport(bool enable)
    {
        m_updateReportEnabled = enable;
        if (!m_updateReportEnabled)
            m_updateReport.clear();
    }

    LogicEngineReport LogicEngineImpl::getLastUpdateReport() const
    {
        return LogicEngineReport{ std::make_unique<LogicEngineReportImpl>(m_updateReport, *m_apiObjects) };
    }

    void LogicEngineImpl::setStatisticsLoggingRate(size_t loggingRate)
    {
        m_statistics.setLoggingRate(loggingRate);
        m_statisticsEnabled = (loggingRate != 0u);
    }

    void LogicEngineImpl::setStatisticsLogLevel(ELogMessageType logLevel)
    {
        m_statistics.setLogLevel(logLevel);
    }

    template DataArray* LogicEngineImpl::createDataArray<float>(const std::vector<float>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec2f>(const std::vector<vec2f>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec3f>(const std::vector<vec3f>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec4f>(const std::vector<vec4f>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<int32_t>(const std::vector<int32_t>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec2i>(const std::vector<vec2i>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec3i>(const std::vector<vec3i>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec4i>(const std::vector<vec4i>&, std::string_view name);
}
