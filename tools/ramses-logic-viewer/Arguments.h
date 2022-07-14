//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/StdFilesystemWrapper.h"
#include <iostream>
#include <vector>
#include <string>
#include <CLI/CLI.hpp>
#include "ramses-framework-api/RamsesFrameworkTypes.h"
#include "ramses-logic/ELogMessageType.h"

class Arguments
{
public:
    void registerOptions(CLI::App& cli)
    {
        cli.description(R"(
Loads and shows a ramses scene from the <ramsesfile>.
<logicfile> and <luafile> are auto-resolved if matching files with *.rlogic and *.lua extensions are found in the same path as <ramsesfile>. (Explicit arguments override autodetection.)
)");
        cli.add_option("ramsesfile", m_sceneFile, "Ramses scene file")->required()->check(CLI::ExistingFile);
        cli.add_option("logicfile", m_logicFile, "Ramses Logic file")->check(CLI::ExistingFile);
        cli.add_option("luafile", m_luaFile, "Lua configuration file")->check(CLI::ExistingFile);
        auto exec = cli.add_option("--exec", m_luaFunction, "Calls the given lua function and exits.");
        auto setWriteConfig = [&](const std::string& filename) {
            m_luaFile     = filename;
            m_writeConfig = true;
        };
        cli.add_option_function<std::string>("--write-config", setWriteConfig, "Writes the default lua configuration file and exits")
            ->expected(0, 1)
            ->type_name("[FILE]")
            ->excludes(exec);
        cli.add_flag("--no-offscreen", m_noOffscreen, "Renders the scene directly to the window's framebuffer. Screenshot size will be the current window size.");

        std::vector<std::pair<std::string, ramses::ELogLevel>> loglevelMap{{"off", ramses::ELogLevel::Off},
                                                             {"fatal", ramses::ELogLevel::Fatal},
                                                             {"error", ramses::ELogLevel::Error},
                                                             {"warn", ramses::ELogLevel::Warn},
                                                             {"info", ramses::ELogLevel::Info},
                                                             {"debug", ramses::ELogLevel::Debug},
                                                             {"trace", ramses::ELogLevel::Trace}};
        cli.add_option("--log-level-console", m_ramsesLogLevel, "Sets log level for console messages.")->transform(CLI::CheckedTransformer(loglevelMap))->default_val(m_ramsesLogLevel);
    }

    const std::string& sceneFile() const
    {
        return m_sceneFile;
    }

    const std::string& logicFile() const
    {
        if (m_logicFile.empty())
        {
            m_logicFile = FindOther(m_sceneFile, "rlogic");
        }
        return m_logicFile;
    }

    const std::string& luaFile() const
    {
        if (m_luaFile.empty())
        {
            m_luaFile = FindOther(m_sceneFile, "lua");
        }
        return m_luaFile;
    }

    const std::string& luaFunction() const
    {
        return m_luaFunction;
    }

    bool noOffscreen() const
    {
        return m_noOffscreen;
    }

    bool writeConfig() const
    {
        return m_writeConfig;
    }

    ramses::ELogLevel ramsesLogLevel() const
    {
        return m_ramsesLogLevel;
    }

    rlogic::ELogMessageType ramsesLogicLogLevel() const
    {
        switch (m_ramsesLogLevel)
        {
        case ramses::ELogLevel::Off:
            return rlogic::ELogMessageType::Off;
        case ramses::ELogLevel::Fatal:
            return rlogic::ELogMessageType::Fatal;
        case ramses::ELogLevel::Error:
            return rlogic::ELogMessageType::Error;
        case ramses::ELogLevel::Warn:
            return rlogic::ELogMessageType::Warn;
        case ramses::ELogLevel::Info:
            return rlogic::ELogMessageType::Info;
        case ramses::ELogLevel::Debug:
            return rlogic::ELogMessageType::Debug;
        case ramses::ELogLevel::Trace:
            return rlogic::ELogMessageType::Trace;
        }

        return rlogic::ELogMessageType::Error;
    }

private:
    [[nodiscard]] static std::string FindOther(fs::path existing, const std::string& extension)
    {
        std::string retval;
        existing.replace_extension(extension);
        if (fs::exists(existing))
        {
            retval = existing.string();
        }
        return retval;
    }

    std::string m_sceneFile;
    mutable std::string m_logicFile;
    mutable std::string m_luaFile;
    std::string m_luaFunction;
    bool m_noOffscreen = false;
    bool m_writeConfig = false;
    ramses::ELogLevel m_ramsesLogLevel = ramses::ELogLevel::Error;
};

