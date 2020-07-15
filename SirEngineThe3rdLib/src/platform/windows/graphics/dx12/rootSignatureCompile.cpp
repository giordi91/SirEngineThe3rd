#include "platform/windows/graphics/dx12/rootSignatureCompile.h"

#include <cassert>
#include <string>
#include <unordered_map>

#include "SirEngine/PSOManager.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"

namespace SirEngine::dx12 {
enum class SUB_ROOT_TYPES {
  CONSTANT,
  DESCRIPTOR_TABLE,
  UAV,
  SRV,
  CBV,
  NULL_TYPE
};

const std::string ROOT_KEY_CONFIG = "config";
const std::string ROOT_KEY_NAME = "name";
const std::string ROOT_KEY_TYPE = "type";
const std::string ROOT_KEY_DATA = "data";
const std::string ROOT_KEY_NUM_DESCRIPTOR = "numDescriptors";
const std::string ROOT_KEY_RANGES = "ranges";
const std::string ROOT_KEY_REGISTER = "register";
const std::string ROOT_KEY_BASE_REGISTER = "baseRegister";
const std::string ROOT_KEY_SIZE_IN_32_BIT_VALUES = "sizeIn32BitValues";
const std::string ROOT_KEY_VISIBILITY = "visibility";
const std::string ROOT_KEY_FLAGS = "flags";
const std::string ROOT_KEY_FLAGS_LOCAL = "local";
const std::string ROOT_EMPTY = "empty";
const std::string ROOT_DEFAULT_STRING = "";
const std::string ROOT_KEY_STATIC_SAMPLERS = "staticSamplers";
const std::string DEFAULT_STRING = "";

static const uint32_t ENGINE_RESIGSTER_SPACE = 0;

const std::unordered_map<std::string, SUB_ROOT_TYPES> STRING_TO_ROOT_SUB_TYPE{
    {"constant", SUB_ROOT_TYPES::CONSTANT},
    {"descriptorTable", SUB_ROOT_TYPES::DESCRIPTOR_TABLE},
    {"UAV", SUB_ROOT_TYPES::UAV},
    {"SRV", SUB_ROOT_TYPES::SRV},
    {"CBV", SUB_ROOT_TYPES::CBV}};

const std::unordered_map<std::string, ROOT_TYPE> STRING_TO_ROOT_TYPE{
    {"RASTER", ROOT_TYPE::RASTER},
    {"COMPUTE", ROOT_TYPE::COMPUTE},
    {"DXR", ROOT_TYPE::DXR},
};

const std::unordered_map<std::string, D3D12_SHADER_VISIBILITY>
    STRING_TO_VISIBILITY_FLAG{{"ALL", D3D12_SHADER_VISIBILITY_ALL},
                              {"VERTEX", D3D12_SHADER_VISIBILITY_VERTEX},
                              {"HULL", D3D12_SHADER_VISIBILITY_HULL},
                              {"DOMAIN", D3D12_SHADER_VISIBILITY_DOMAIN},
                              {"GEOMETRY", D3D12_SHADER_VISIBILITY_GEOMETRY},
                              {"PIXEL", D3D12_SHADER_VISIBILITY_PIXEL}};

SUB_ROOT_TYPES
getTypeEnum(const std::string &type) {
  auto found = STRING_TO_ROOT_SUB_TYPE.find(type);
  if (found != STRING_TO_ROOT_SUB_TYPE.end()) {
    return found->second;
  }
  assert(0 && "could not convert root type from string to enum");
  return SUB_ROOT_TYPES::NULL_TYPE;
}
ROOT_TYPE getFileTypeEnum(const std::string &type) {
  auto found = STRING_TO_ROOT_TYPE.find(type);
  if (found != STRING_TO_ROOT_TYPE.end()) {
    return found->second;
  }
  assert(0 && "could not convert root type from string to enum");
  return ROOT_TYPE::NULL_TYPE;
}

D3D12_SHADER_VISIBILITY getVisibility(const nlohmann::json &jobj) {
  const std::string visString =
      getValueIfInJson(jobj, ROOT_KEY_VISIBILITY, ROOT_DEFAULT_STRING);
  D3D12_SHADER_VISIBILITY toReturn = D3D12_SHADER_VISIBILITY_ALL;
  if (!visString.empty()) {
    auto found = STRING_TO_VISIBILITY_FLAG.find(visString);
    if (found != STRING_TO_VISIBILITY_FLAG.end()) {
      toReturn = found->second;
    } else {
      SE_CORE_WARN(
          "Cannot convert shader visibility from string, value not "
          "recognized: {0}",
          visString);
      SE_CORE_WARN("D3D12_SHADER_VISIBILITY_ALL will be used");
    }
  }
  return toReturn;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> getStaticSamplers() {
  // Applications usually only need a handful of samplers.  So just define them
  // all up front and keep them available as part of the root signature.

  const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
      0,                                 // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP);  // addressW

  const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
      1,                                  // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_POINT,     // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP);  // addressW

  const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
      2,                                 // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP);  // addressW

  const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
      3,                                  // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_LINEAR,    // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP);  // addressW

  const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
      4,                                // shaderRegister
      D3D12_FILTER_ANISOTROPIC,         // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
      0.0f,                             // mipLODBias
      8);                               // maxAnisotropy

  const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
      5,                                 // shaderRegister
      D3D12_FILTER_ANISOTROPIC,          // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
      0.0f,                              // mipLODBias
      8);                                // maxAnisotropy
  const CD3DX12_STATIC_SAMPLER_DESC shadowPCFClamp(
      6,                                                 // shaderRegister
      D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,  // filter
      D3D12_TEXTURE_ADDRESS_MODE_BORDER,                 // addressU
      D3D12_TEXTURE_ADDRESS_MODE_BORDER,                 // addressV
      D3D12_TEXTURE_ADDRESS_MODE_BORDER,                 // addressW
      0.0f,                                              // mipLODBias
      16, D3D12_COMPARISON_FUNC_GREATER,
      D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);  // maxAnisotropy

  return {pointWrap,       pointClamp,       linearWrap,    linearClamp,
          anisotropicWrap, anisotropicClamp, shadowPCFClamp};
}

void processSRV(nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param,
                bool useRegister) {
  int defaultInt = -1;
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto &jdata = jobj[ROOT_KEY_DATA];

  int startRegister =
      getValueIfInJson(jdata, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  param.InitAsShaderResourceView(
      startRegister, useRegister ? PSOManager::PER_OBJECT_BINDING_INDEX : 0,
      getVisibility(jobj));
}
void processCBV(nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param,
                bool useRegister) {
  int defaultInt = -1;
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto &jdata = jobj[ROOT_KEY_DATA];

  int startRegister =
      getValueIfInJson(jdata, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  param.InitAsConstantBufferView(
      startRegister, useRegister ? PSOManager::PER_OBJECT_BINDING_INDEX : 0,
      getVisibility(jobj));
}
void processUAV(nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param,
                bool useRegister) {
  int defaultInt = -1;
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto &jdata = jobj[ROOT_KEY_DATA];

  int startRegister =
      getValueIfInJson(jdata, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  param.InitAsUnorderedAccessView(
      startRegister, useRegister ? PSOManager::PER_OBJECT_BINDING_INDEX : 0,
      getVisibility(jobj));
}

void processConstant(nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param,
                     bool useRegister) {
  int defaultInt = -1;
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto &jdata = jobj[ROOT_KEY_DATA];

  int sizeIn32Bit =
      getValueIfInJson(jdata, ROOT_KEY_SIZE_IN_32_BIT_VALUES, defaultInt);
  assert(sizeIn32Bit != -1);
  int startRegister =
      getValueIfInJson(jdata, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  param.InitAsConstants(sizeIn32Bit, startRegister,
                        useRegister ? PSOManager::PER_OBJECT_BINDING_INDEX : 0,
                        getVisibility(jobj));
}

void initDescriptorAsUAV(nlohmann::json &jobj,
                         CD3DX12_DESCRIPTOR_RANGE &descriptor,
                         bool useRegister) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, count, startRegister,
                  useRegister ? PSOManager::PER_OBJECT_BINDING_INDEX : 0);
}

void initDescriptorAsSRV(nlohmann::json &jobj,
                         CD3DX12_DESCRIPTOR_RANGE &descriptor,
                         bool useRegister) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, count, startRegister,
                  useRegister ? PSOManager::PER_OBJECT_BINDING_INDEX : 0);
}

void initDescriptorAsConstant(nlohmann::json &jobj,
                              CD3DX12_DESCRIPTOR_RANGE &descriptor,
                              bool useRegister) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, count, startRegister,
                  useRegister ? PSOManager::PER_OBJECT_BINDING_INDEX : 0);
}

void processDescriptorTable(nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param,
                            bool useRegister) {
  size_t size = jobj.size();
  assert(size != 0);
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto dataJ = jobj[ROOT_KEY_DATA];
  assert(dataJ.find(ROOT_KEY_RANGES) != dataJ.end());
  auto rangesJ = dataJ[ROOT_KEY_RANGES];
  // TODO clean this up please
  auto *ranges = new CD3DX12_DESCRIPTOR_RANGE[rangesJ.size()];
  int counter = 0;
  const std::string defaultString = "";
  for (auto &subRange : rangesJ) {
    std::string typeString =
        getValueIfInJson(subRange, ROOT_KEY_TYPE, defaultString);
    SUB_ROOT_TYPES descriptorType = getTypeEnum(typeString);
    switch (descriptorType) {
      case (SUB_ROOT_TYPES::UAV): {
        initDescriptorAsUAV(subRange, ranges[counter], useRegister);
        break;
      }
      case (SUB_ROOT_TYPES::SRV): {
        initDescriptorAsSRV(subRange, ranges[counter], useRegister);
        break;
      }
      case (SUB_ROOT_TYPES::CBV): {
        initDescriptorAsConstant(subRange, ranges[counter], useRegister);
        break;
      }
      default: {
      }
    }
    ++counter;
  }
  param.InitAsDescriptorTable(counter, ranges);
}
ID3DBlob *serializeRootSignature(D3D12_ROOT_SIGNATURE_DESC &desc) {
  ID3DBlob *blob;
  ID3DBlob *error;
  D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob,
                              &error);
  if (error != nullptr) {
    SE_CORE_ERROR("Error serializing root signature :{0}",
                  static_cast<char *>(error->GetBufferPointer()));
    blob = nullptr;
  }
  return blob;
}

bool shouldBindSamplers(const nlohmann::json &jobj) {
  bool toReturn = false;
  auto found = jobj.find(ROOT_KEY_STATIC_SAMPLERS);
  if (found != jobj.end()) {
    toReturn = found.value().get<bool>();
  }
  return toReturn;
}

ID3D12RootSignature *enginePerFrameEmptyRS(const char *name) {
  int extraRegister = 1;

  // we have one flat descriptor table to bind
  uint32_t registerCount = 1;
  std::vector<CD3DX12_ROOT_PARAMETER> rootParams(registerCount + extraRegister);
  CD3DX12_DESCRIPTOR_RANGE ranges{};
  // create constant buffer for camera values
  int startRegister = 0;
  ranges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, startRegister,
              ENGINE_RESIGSTER_SPACE);
  rootParams[0].InitAsDescriptorTable(1, &ranges);

  auto samplers = getStaticSamplers();

  // bind staticSamplers
  D3D12_STATIC_SAMPLER_DESC const *staticSamplers = &samplers[0];
  UINT numStaticSampers = static_cast<UINT>(samplers.size());

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
      static_cast<UINT>(rootParams.size()), rootParams.data(), numStaticSampers,
      staticSamplers);

  const ROOT_TYPE fileTypeEnum = ROOT_TYPE::RASTER;
  ID3DBlob *blob = nullptr;
  blob = serializeRootSignature(rootSignatureDesc);

  ID3D12RootSignature *rootSig;
  const HRESULT res = SirEngine::dx12::DEVICE->CreateRootSignature(
      1, blob->GetBufferPointer(), blob->GetBufferSize(),
      IID_PPV_ARGS(&(rootSig)));
  assert(res == S_OK);
  blob->Release();
  rootSig->SetName(frameConvertWide(name));
  return rootSig;
}

RootCompilerResult flatTablesRS(nlohmann::json jobj, const std::string &name,
                                ID3DBlob **blob) {
  if (name == "forwardPhongRS") {
    int x = 0;
  }
  const std::string tempKey = "useRegisterSpace";
  const std::string tempKey2 = "passConfig";
  bool useRegister = getValueIfInJson(jobj, tempKey, false);

  // not using register not supported anymore, will be fixed over time
  assert(useRegister == true);

  bool hasPassConfig = jobj.find(tempKey2) != jobj.end();
  if (hasPassConfig) {
    hasPassConfig = jobj[tempKey2].size() > 0 ? true : false;
  }
  bool hasConfig = jobj.find(ROOT_KEY_CONFIG) != jobj.end();

  std::string fileType = getValueIfInJson(jobj, ROOT_KEY_TYPE, DEFAULT_STRING);
  assert(!fileType.empty());

  // we have one flat descriptor table to bind
  uint32_t registerCount = 1;
  registerCount += hasPassConfig ? 1 : 0;
  registerCount += hasConfig ? 1 : 0;
  std::vector<CD3DX12_ROOT_PARAMETER> rootParams(registerCount);
  CD3DX12_DESCRIPTOR_RANGE ranges{};
  if (useRegister) {
    // create constant buffer for camera values
    int startRegister = 0;
    ranges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, startRegister,
                ENGINE_RESIGSTER_SPACE);
    rootParams[0].InitAsDescriptorTable(1, &ranges);
  }
  int configIndex =
      0 + static_cast<int>(hasPassConfig) + static_cast<int>(hasConfig);

  int userCounter = 0;
  if (hasConfig) {
    auto &configValue = jobj[ROOT_KEY_CONFIG];
    auto *userRanges = new CD3DX12_DESCRIPTOR_RANGE[configValue.size()]{};
    for (auto &subConfig : configValue) {
      // getting the type of root
      std::string configTypeValueString =
          getValueIfInJson(subConfig, ROOT_KEY_TYPE, DEFAULT_STRING);
      assert(!configTypeValueString.empty());
      SUB_ROOT_TYPES configType = getTypeEnum(configTypeValueString);
      // here we are dealing with the old system, where everything was a
      // descriptor table now we want to unpack that, so we need to extract the
      // ranges from inside the descriptor tables which are always one object,
      // and flatten them out
      assert(configType == SUB_ROOT_TYPES::DESCRIPTOR_TABLE);

      // lets go in and expand the actual inner type,accessing the ranges
      assert(subConfig.find(ROOT_KEY_DATA) != subConfig.end());
      auto dataJ = subConfig[ROOT_KEY_DATA];
      assert(dataJ.find(ROOT_KEY_RANGES) != dataJ.end());
      auto rangesJ = dataJ[ROOT_KEY_RANGES];
      assert(rangesJ.size() == 1);
      auto currentRange = rangesJ[0];
      configTypeValueString =
          getValueIfInJson(currentRange, ROOT_KEY_TYPE, DEFAULT_STRING);
      assert(!configTypeValueString.empty());
      configType = getTypeEnum(configTypeValueString);

      switch (configType) {
        case (SUB_ROOT_TYPES::CBV): {
          initDescriptorAsConstant(currentRange, userRanges[userCounter],
                                   useRegister);
          break;
        }
        case (SUB_ROOT_TYPES::SRV): {
          initDescriptorAsSRV(currentRange, userRanges[userCounter],
                              useRegister);
          break;
        }
        case (SUB_ROOT_TYPES::UAV): {
          initDescriptorAsUAV(currentRange, userRanges[userCounter],
                              useRegister);
          break;
        }
        default: {
          assert(0 && "descriptor type not supported");
        }
      }
      ++userCounter;
    }
    rootParams[configIndex].InitAsDescriptorTable(userCounter, userRanges);
  }

  UINT numStaticSampers = 0;
  D3D12_STATIC_SAMPLER_DESC const *staticSamplers = nullptr;
  auto samplers = getStaticSamplers();

  if (shouldBindSamplers(jobj)) {
    staticSamplers = &samplers[0];
    numStaticSampers = static_cast<UINT>(samplers.size());
  }

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
      static_cast<UINT>(rootParams.size()), rootParams.data(), numStaticSampers,
      staticSamplers);

  // process flags
  auto found = jobj.find(ROOT_KEY_FLAGS);
  if (found != jobj.end()) {
    auto &jflags = jobj[ROOT_KEY_FLAGS];
    for (auto &subFlag : jflags) {
      // This was for DXR, which is not supported yet
      if (subFlag.get<std::string>() == ROOT_KEY_FLAGS_LOCAL) {
        rootSignatureDesc.Flags |=
            D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
      }
      // in case we want input assembler, this is by now legacy
      // but keeping the feature just in case. This way is not particularly
      // pretty maybe there is a better way to do this, maybe converting with a
      // macro
      if (subFlag.get<std::string>() ==
          "D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT") {
        rootSignatureDesc.Flags |=
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
      }
    }
  }

  const ROOT_TYPE fileTypeEnum = getFileTypeEnum(fileType);
  (*blob) = serializeRootSignature(rootSignatureDesc);

  return RootCompilerResult{
      frameString(name.c_str()),
      nullptr,
      fileTypeEnum,
      true,
      static_cast<uint16_t>(userCounter),
      {0, -1, hasPassConfig ? 1 : -1, hasPassConfig ? 2 : 1}};
}

RootCompilerResult multipleTablesRS(nlohmann::json jobj,
                                    const std::string &name, ID3DBlob **blob) {
  // assert(0);
  const std::string tempKey = "useRegisterSpace";
  bool useRegister = getValueIfInJson(jobj, tempKey, false);

  int extraRegister = useRegister ? 1 : 0;

  auto &configValue = jobj[ROOT_KEY_CONFIG];

  std::string fileType = getValueIfInJson(jobj, ROOT_KEY_TYPE, DEFAULT_STRING);
  assert(!fileType.empty());

  std::vector<CD3DX12_ROOT_PARAMETER> rootParams(configValue.size() +
                                                 extraRegister);
  CD3DX12_DESCRIPTOR_RANGE ranges{};
  if (useRegister) {
    // create constant buffer for camera values
    int startRegister = 0;
    ranges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, startRegister,
                ENGINE_RESIGSTER_SPACE);
    rootParams[0].InitAsDescriptorTable(1, &ranges);
  }
  int counter = 0 + extraRegister;
  for (auto &subConfig : configValue) {
    // getting the type of root
    std::string configTypeValueString =
        getValueIfInJson(subConfig, ROOT_KEY_TYPE, DEFAULT_STRING);
    assert(!configTypeValueString.empty());
    SUB_ROOT_TYPES configType = getTypeEnum(configTypeValueString);
    switch (configType) {
      case (SUB_ROOT_TYPES::CONSTANT): {
        processConstant(subConfig, rootParams[counter], useRegister);
        break;
      }
      case (SUB_ROOT_TYPES::DESCRIPTOR_TABLE): {
        processDescriptorTable(subConfig, rootParams[counter], useRegister);
        break;
      }
      case (SUB_ROOT_TYPES::SRV): {
        processSRV(subConfig, rootParams[counter], useRegister);
        break;
      }
      case (SUB_ROOT_TYPES::CBV): {
        processCBV(subConfig, rootParams[counter], useRegister);
        break;
      }
      case (SUB_ROOT_TYPES::UAV): {
        processUAV(subConfig, rootParams[counter], useRegister);
        break;
      }
      default: {
        assert(0 && " root type not supported");
      }
    }
    ++counter;
  }

  UINT numStaticSampers = 0;
  D3D12_STATIC_SAMPLER_DESC const *staticSamplers = nullptr;
  auto samplers = getStaticSamplers();

  if (shouldBindSamplers(jobj)) {
    staticSamplers = &samplers[0];
    numStaticSampers = static_cast<UINT>(samplers.size());
  }

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
      static_cast<UINT>(rootParams.size()), rootParams.data(), numStaticSampers,
      staticSamplers);

  // process flags
  auto found = jobj.find(ROOT_KEY_FLAGS);
  if (found != jobj.end()) {
    auto &jflags = jobj[ROOT_KEY_FLAGS];
    for (auto &subFlag : jflags) {
      if (subFlag.get<std::string>() == ROOT_KEY_FLAGS_LOCAL) {
        rootSignatureDesc.Flags |=
            D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
      }
      // TODO not exactly pretty, can we get a way with an automatic
      // conversion? maybe an X macro?
      if (subFlag.get<std::string>() ==
          "D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT") {
        rootSignatureDesc.Flags |=
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
      }
    }
  }

  const ROOT_TYPE fileTypeEnum = getFileTypeEnum(fileType);
  (*blob) = serializeRootSignature(rootSignatureDesc);

  return RootCompilerResult{frameString(name.c_str()), nullptr, fileTypeEnum,
                            false, 0};
}

RootCompilerResult processSignatureFileToBlob(const char *path,
                                              ID3DBlob **blob) {
  auto jobj = getJsonObj(path);
  // there might be multiple signatures in the files lets loop them

  const std::string defaultString;
  const std::string &name = getFileName(path);

  // assert(jobj.find(ROOT_KEY_CONFIG) != jobj.end());

  const std::string flatDescriptorTableKey = "flatDescriptorTable";
  bool isFlatTable = getValueIfInJson(jobj, flatDescriptorTableKey, false);
  if (isFlatTable) {
    return flatTablesRS(jobj, name, blob);
  } else {
    return multipleTablesRS(jobj, name, blob);
  }
}

RootCompilerResult processSignatureFile(const char *path) {
  ID3D12RootSignature *rootSig;
  ID3DBlob *blob;
  RootCompilerResult compilerResult = processSignatureFileToBlob(path, &blob);
  const HRESULT res = SirEngine::dx12::DEVICE->CreateRootSignature(
      1, blob->GetBufferPointer(), blob->GetBufferSize(),
      IID_PPV_ARGS(&(rootSig)));
  assert(res == S_OK);
  blob->Release();
  compilerResult.root = rootSig;
  return compilerResult;
}
}  // namespace SirEngine::dx12
