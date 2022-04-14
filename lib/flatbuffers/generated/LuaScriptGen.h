// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_LUASCRIPT_RLOGIC_SERIALIZATION_H_
#define FLATBUFFERS_GENERATED_LUASCRIPT_RLOGIC_SERIALIZATION_H_

#include "flatbuffers/flatbuffers.h"

#include "LogicObjectGen.h"
#include "LuaModuleGen.h"
#include "PropertyGen.h"

namespace rlogic_serialization {

struct LuaScript;
struct LuaScriptBuilder;

inline const flatbuffers::TypeTable *LuaScriptTypeTable();

struct LuaScript FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef LuaScriptBuilder Builder;
  struct Traits;
  static const flatbuffers::TypeTable *MiniReflectTypeTable() {
    return LuaScriptTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_BASE = 4,
    VT_LUASOURCECODE = 6,
    VT_USERMODULES = 8,
    VT_STANDARDMODULES = 10,
    VT_ROOTINPUT = 12,
    VT_ROOTOUTPUT = 14
  };
  const rlogic_serialization::LogicObject *base() const {
    return GetPointer<const rlogic_serialization::LogicObject *>(VT_BASE);
  }
  const flatbuffers::String *luaSourceCode() const {
    return GetPointer<const flatbuffers::String *>(VT_LUASOURCECODE);
  }
  const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::LuaModuleUsage>> *userModules() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::LuaModuleUsage>> *>(VT_USERMODULES);
  }
  const flatbuffers::Vector<uint8_t> *standardModules() const {
    return GetPointer<const flatbuffers::Vector<uint8_t> *>(VT_STANDARDMODULES);
  }
  const rlogic_serialization::Property *rootInput() const {
    return GetPointer<const rlogic_serialization::Property *>(VT_ROOTINPUT);
  }
  const rlogic_serialization::Property *rootOutput() const {
    return GetPointer<const rlogic_serialization::Property *>(VT_ROOTOUTPUT);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_BASE) &&
           verifier.VerifyTable(base()) &&
           VerifyOffset(verifier, VT_LUASOURCECODE) &&
           verifier.VerifyString(luaSourceCode()) &&
           VerifyOffset(verifier, VT_USERMODULES) &&
           verifier.VerifyVector(userModules()) &&
           verifier.VerifyVectorOfTables(userModules()) &&
           VerifyOffset(verifier, VT_STANDARDMODULES) &&
           verifier.VerifyVector(standardModules()) &&
           VerifyOffset(verifier, VT_ROOTINPUT) &&
           verifier.VerifyTable(rootInput()) &&
           VerifyOffset(verifier, VT_ROOTOUTPUT) &&
           verifier.VerifyTable(rootOutput()) &&
           verifier.EndTable();
  }
};

struct LuaScriptBuilder {
  typedef LuaScript Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_base(flatbuffers::Offset<rlogic_serialization::LogicObject> base) {
    fbb_.AddOffset(LuaScript::VT_BASE, base);
  }
  void add_luaSourceCode(flatbuffers::Offset<flatbuffers::String> luaSourceCode) {
    fbb_.AddOffset(LuaScript::VT_LUASOURCECODE, luaSourceCode);
  }
  void add_userModules(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::LuaModuleUsage>>> userModules) {
    fbb_.AddOffset(LuaScript::VT_USERMODULES, userModules);
  }
  void add_standardModules(flatbuffers::Offset<flatbuffers::Vector<uint8_t>> standardModules) {
    fbb_.AddOffset(LuaScript::VT_STANDARDMODULES, standardModules);
  }
  void add_rootInput(flatbuffers::Offset<rlogic_serialization::Property> rootInput) {
    fbb_.AddOffset(LuaScript::VT_ROOTINPUT, rootInput);
  }
  void add_rootOutput(flatbuffers::Offset<rlogic_serialization::Property> rootOutput) {
    fbb_.AddOffset(LuaScript::VT_ROOTOUTPUT, rootOutput);
  }
  explicit LuaScriptBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  LuaScriptBuilder &operator=(const LuaScriptBuilder &);
  flatbuffers::Offset<LuaScript> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<LuaScript>(end);
    return o;
  }
};

inline flatbuffers::Offset<LuaScript> CreateLuaScript(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<rlogic_serialization::LogicObject> base = 0,
    flatbuffers::Offset<flatbuffers::String> luaSourceCode = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::LuaModuleUsage>>> userModules = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint8_t>> standardModules = 0,
    flatbuffers::Offset<rlogic_serialization::Property> rootInput = 0,
    flatbuffers::Offset<rlogic_serialization::Property> rootOutput = 0) {
  LuaScriptBuilder builder_(_fbb);
  builder_.add_rootOutput(rootOutput);
  builder_.add_rootInput(rootInput);
  builder_.add_standardModules(standardModules);
  builder_.add_userModules(userModules);
  builder_.add_luaSourceCode(luaSourceCode);
  builder_.add_base(base);
  return builder_.Finish();
}

struct LuaScript::Traits {
  using type = LuaScript;
  static auto constexpr Create = CreateLuaScript;
};

inline flatbuffers::Offset<LuaScript> CreateLuaScriptDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<rlogic_serialization::LogicObject> base = 0,
    const char *luaSourceCode = nullptr,
    const std::vector<flatbuffers::Offset<rlogic_serialization::LuaModuleUsage>> *userModules = nullptr,
    const std::vector<uint8_t> *standardModules = nullptr,
    flatbuffers::Offset<rlogic_serialization::Property> rootInput = 0,
    flatbuffers::Offset<rlogic_serialization::Property> rootOutput = 0) {
  auto luaSourceCode__ = luaSourceCode ? _fbb.CreateString(luaSourceCode) : 0;
  auto userModules__ = userModules ? _fbb.CreateVector<flatbuffers::Offset<rlogic_serialization::LuaModuleUsage>>(*userModules) : 0;
  auto standardModules__ = standardModules ? _fbb.CreateVector<uint8_t>(*standardModules) : 0;
  return rlogic_serialization::CreateLuaScript(
      _fbb,
      base,
      luaSourceCode__,
      userModules__,
      standardModules__,
      rootInput,
      rootOutput);
}

inline const flatbuffers::TypeTable *LuaScriptTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_SEQUENCE, 0, 0 },
    { flatbuffers::ET_STRING, 0, -1 },
    { flatbuffers::ET_SEQUENCE, 1, 1 },
    { flatbuffers::ET_UCHAR, 1, -1 },
    { flatbuffers::ET_SEQUENCE, 0, 2 },
    { flatbuffers::ET_SEQUENCE, 0, 2 }
  };
  static const flatbuffers::TypeFunction type_refs[] = {
    rlogic_serialization::LogicObjectTypeTable,
    rlogic_serialization::LuaModuleUsageTypeTable,
    rlogic_serialization::PropertyTypeTable
  };
  static const char * const names[] = {
    "base",
    "luaSourceCode",
    "userModules",
    "standardModules",
    "rootInput",
    "rootOutput"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_TABLE, 6, type_codes, type_refs, nullptr, names
  };
  return &tt;
}

}  // namespace rlogic_serialization

#endif  // FLATBUFFERS_GENERATED_LUASCRIPT_RLOGIC_SERIALIZATION_H_
