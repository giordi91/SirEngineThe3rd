#include "platform/windows/graphics/dx12/rootSignatureCompile.h"

#include <cassert>
#include <string>
#include <unordered_map>

#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/materialMetadata.h"
#include "SirEngine/log.h"
#include "SirEngine/psoManager.h"
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
const std::string ROOT_KEY_PASS_CONFIG = "passConfig";
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

SUB_ROOT_TYPES
getTypeEnum(graphics::MATERIAL_RESOURCE_TYPE type,
            graphics::MATERIAL_RESOURCE_FLAGS flags) {
  switch (type) {
    case graphics::MATERIAL_RESOURCE_TYPE::TEXTURE: {
      return SUB_ROOT_TYPES::SRV;
    }
    case graphics::MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER: {
      return SUB_ROOT_TYPES::CBV;
    }
    case graphics::MATERIAL_RESOURCE_TYPE::BUFFER: {
      return (static_cast<uint32_t>(flags) &
              static_cast<uint32_t>(
                  graphics::MATERIAL_RESOURCE_FLAGS::READ_ONLY) > 0)
                 ? SUB_ROOT_TYPES::SRV
                 : SUB_ROOT_TYPES::UAV;
    }
    default: {
      assert(0 && "descriptor type not suppoerted");
      return SUB_ROOT_TYPES::NULL_TYPE;
    }
  }
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

static D3D12_SAMPLER_DESC STATIC_SAMPLERS[7];
const D3D12_SAMPLER_DESC *getSamplers() {
  // Applications usually only need a handful of samplers.  So just define them
  // all up front and keep them available as part of the root signature.

  D3D12_SAMPLER_DESC pointWrap{
      D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      0.0f,
      1,
      D3D12_COMPARISON_FUNC_ALWAYS,
      {},
      0,
      D3D12_FLOAT32_MAX,
  };  // addressW

  const D3D12_SAMPLER_DESC pointClamp{
      D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
      0.0f,
      1,
      D3D12_COMPARISON_FUNC_ALWAYS,
      {},
      0,
      D3D12_FLOAT32_MAX,
  };

  const D3D12_SAMPLER_DESC linearWrap{
      D3D12_FILTER_MIN_MAG_MIP_LINEAR,  // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      0.0f,
      1,
      D3D12_COMPARISON_FUNC_ALWAYS,
      {},
      0,
      D3D12_FLOAT32_MAX};  // addressW

  const D3D12_SAMPLER_DESC linearClamp{
      D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
      0.0f,
      1,
      D3D12_COMPARISON_FUNC_ALWAYS,
      {},
      0,
      D3D12_FLOAT32_MAX};

  const D3D12_SAMPLER_DESC anisotropicWrap{
      D3D12_FILTER_ANISOTROPIC,         // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
      0.0f,
      8,
      D3D12_COMPARISON_FUNC_ALWAYS,
      {},
      0,
      D3D12_FLOAT32_MAX};

  const D3D12_SAMPLER_DESC anisotropicClamp{
      D3D12_FILTER_ANISOTROPIC,          // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
      0.0f,
      8,
      D3D12_COMPARISON_FUNC_ALWAYS,
      {},
      0,
      D3D12_FLOAT32_MAX};

  const D3D12_SAMPLER_DESC shadowPCFClamp{
      D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,  // filter
      D3D12_TEXTURE_ADDRESS_MODE_BORDER,                 // addressU
      D3D12_TEXTURE_ADDRESS_MODE_BORDER,                 // addressV
      D3D12_TEXTURE_ADDRESS_MODE_BORDER,                 // addressW
      0.0f,                                              // mipLODBias
      16,
      D3D12_COMPARISON_FUNC_GREATER,
      D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK};  // maxAnisotropy

  STATIC_SAMPLERS[0] = pointWrap;
  STATIC_SAMPLERS[1] = pointClamp;
  STATIC_SAMPLERS[2] = linearWrap;
  STATIC_SAMPLERS[3] = linearClamp;
  STATIC_SAMPLERS[4] = anisotropicWrap;
  STATIC_SAMPLERS[5] = anisotropicClamp;
  STATIC_SAMPLERS[6] = shadowPCFClamp;
  return STATIC_SAMPLERS;
  // return {pointWrap,       pointClamp,       linearWrap,    linearClamp,
  //        anisotropicWrap, anisotropicClamp, shadowPCFClamp};
}

void initDescriptorAsUAV(const nlohmann::json &jobj,
                         CD3DX12_DESCRIPTOR_RANGE &descriptor,
                         const uint32_t registerSpace) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, count, startRegister,
                  registerSpace);
}
void initDescriptorAsUAV(const graphics::MaterialResource &metadata,
                         CD3DX12_DESCRIPTOR_RANGE &descriptor) {
  int startRegister = metadata.binding;
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, startRegister,
                  metadata.set);
}

void initDescriptorAsSRV(const nlohmann::json &jobj,
                         CD3DX12_DESCRIPTOR_RANGE &descriptor,
                         const uint32_t registerSpace) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, count, startRegister,
                  registerSpace);
}
void initDescriptorAsSRV(const graphics::MaterialResource &metadata,
                         CD3DX12_DESCRIPTOR_RANGE &descriptor) {
  int startRegister = metadata.binding;
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, startRegister,
                  metadata.set);
}

void initDescriptorAsConstant(const nlohmann::json &jobj,
                              CD3DX12_DESCRIPTOR_RANGE &descriptor,
                              const uint32_t registerSpace) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, count, startRegister,
                  registerSpace);
}
void initDescriptorAsConstant(const graphics::MaterialResource &metadata,
                              CD3DX12_DESCRIPTOR_RANGE &descriptor) {
  int startRegister = metadata.binding;
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, startRegister,
                  metadata.set);
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

void processRootConfiguration(CD3DX12_DESCRIPTOR_RANGE *userRanges,
                              const nlohmann::basic_json<> &subConfig,
                              const uint32_t registerSpace) {
  // getting the type of root
  std::string configTypeValueString =
      getValueIfInJson(subConfig, ROOT_KEY_TYPE, DEFAULT_STRING);
  assert(!configTypeValueString.empty());
  SUB_ROOT_TYPES configType = getTypeEnum(configTypeValueString);

  // lets go in and expand the actual inner type,accessing the ranges
  configTypeValueString =
      getValueIfInJson(subConfig, ROOT_KEY_TYPE, DEFAULT_STRING);
  assert(!configTypeValueString.empty());
  configType = getTypeEnum(configTypeValueString);

  switch (configType) {
    case (SUB_ROOT_TYPES::CBV): {
      initDescriptorAsConstant(subConfig, *userRanges, registerSpace);
      break;
    }
    case (SUB_ROOT_TYPES::SRV): {
      initDescriptorAsSRV(subConfig, *userRanges, registerSpace);
      break;
    }
    case (SUB_ROOT_TYPES::UAV): {
      initDescriptorAsUAV(subConfig, *userRanges, registerSpace);
      break;
    }
    default: {
      assert(0 && "descriptor type not supported");
    }
  }
}
void processRootConfiguration(CD3DX12_DESCRIPTOR_RANGE *userRanges,
                              const graphics::MaterialResource &metadata,
                              const uint32_t registerSpace) {
  SUB_ROOT_TYPES configType = getTypeEnum(metadata.type, metadata.flags);

  switch (configType) {
    case (SUB_ROOT_TYPES::CBV): {
      initDescriptorAsConstant(metadata, *userRanges);
      break;
    }
    case (SUB_ROOT_TYPES::SRV): {
      initDescriptorAsSRV(metadata, *userRanges);
      break;
    }
    case (SUB_ROOT_TYPES::UAV): {
      initDescriptorAsUAV(metadata, *userRanges);
      break;
    }
    default: {
      assert(0 && "descriptor type not supported");
    }
  }
}

void processRootFlags(const nlohmann::json &jobj,
                      CD3DX12_ROOT_SIGNATURE_DESC &rootSignatureDesc) {
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
      // pretty maybe there is a better way to do this, maybe we can have a look
      // up like the rest
      if (subFlag.get<std::string>() ==
          "D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT") {
        rootSignatureDesc.Flags |=
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
      }
    }
  }
}

RootCompilerResult flatTablesRS(nlohmann::json jobj, const std::string &name,
                                ID3DBlob **blob) {
  bool useStaticSampler = shouldBindSamplers(jobj);

  // checking if we have pass definition, if not we shift the indeces
  bool hasPassConfig = jobj.find(ROOT_KEY_PASS_CONFIG) != jobj.end();
  if (hasPassConfig) {
    // we might have a pass defined but is actually an empty array
    hasPassConfig = jobj[ROOT_KEY_PASS_CONFIG].size() > 0 ? true : false;
  }
  bool hasConfig = jobj.find(ROOT_KEY_CONFIG) != jobj.end();

  std::string fileType = getValueIfInJson(jobj, ROOT_KEY_TYPE, DEFAULT_STRING);
  assert(!fileType.empty());

  // we have one flat descriptor table to bind
  uint32_t registerCount = 1;
  registerCount += hasPassConfig ? 1 : 0;
  registerCount += hasConfig ? 1 : 0;
  registerCount += useStaticSampler ? 1 : 0;
  std::vector<CD3DX12_ROOT_PARAMETER> rootParams(registerCount);
  CD3DX12_DESCRIPTOR_RANGE ranges{};

  // create constant buffer for camera values
  int startRegister = 0;
  ranges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, startRegister,
              ENGINE_RESIGSTER_SPACE);
  rootParams[0].InitAsDescriptorTable(1, &ranges);

  // this is per object config
  int configIndex = 0 + useStaticSampler + static_cast<int>(hasPassConfig) +
                    static_cast<int>(hasConfig);

  int userCounter = 0;
  CD3DX12_DESCRIPTOR_RANGE *userRanges = nullptr;
  if (hasConfig) {
    auto &configValue = jobj[ROOT_KEY_CONFIG];
    userRanges = new CD3DX12_DESCRIPTOR_RANGE[configValue.size()]{};
    for (auto &subConfig : configValue) {
      processRootConfiguration(&userRanges[userCounter], subConfig,
                               PSOManager::PER_OBJECT_BINDING_INDEX);
      ++userCounter;
    }
    rootParams[configIndex].InitAsDescriptorTable(userCounter, userRanges);
  }
  CD3DX12_DESCRIPTOR_RANGE *passRanges = nullptr;
  if (hasPassConfig) {
    auto &passConfig = jobj[ROOT_KEY_PASS_CONFIG];
    passRanges = new CD3DX12_DESCRIPTOR_RANGE[passConfig.size()]{};
    int passCounter = 0;
    for (auto &subConfig : passConfig) {
      processRootConfiguration(&passRanges[passCounter], subConfig,
                               PSOManager::PER_PASS_BINDING_INDEX);
      ++passCounter;
    }
    // Magic number here is one because if we have a per pass index the
    // descriptor will be 1, zero will be the per frame data
    rootParams[1ll + useStaticSampler].InitAsDescriptorTable(passCounter,
                                                             passRanges);
  }

  UINT numStaticSampers = 0;
  D3D12_STATIC_SAMPLER_DESC const *staticSamplers = nullptr;

  CD3DX12_DESCRIPTOR_RANGE normalSamplersDesc;
  if (useStaticSampler) {
    const D3D12_SAMPLER_DESC *normalSamplers = getSamplers();
    int samplersCount = static_cast<int>(getSamplersCount());
    int baseRegiser = 0;
    int space = 1;
    normalSamplersDesc.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, samplersCount,
                            baseRegiser, space);

    rootParams[1].InitAsDescriptorTable(1, &normalSamplersDesc);
  }

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
      static_cast<UINT>(rootParams.size()), rootParams.data(), numStaticSampers,
      staticSamplers);

  processRootFlags(jobj, rootSignatureDesc);

  const ROOT_TYPE fileTypeEnum = getFileTypeEnum(fileType);
  (*blob) = serializeRootSignature(rootSignatureDesc);

  return RootCompilerResult{
      frameString(name.c_str()),
      nullptr,
      fileTypeEnum,
      true,
      static_cast<uint16_t>(userCounter),
      {0, useStaticSampler ? 1 : -1,
       static_cast<uint16_t>(hasPassConfig ? 1 + useStaticSampler : -1),
       static_cast<uint16_t>(useStaticSampler + hasPassConfig + 1)}};
}

RootCompilerResult processSignatureFileToBlob(const char *path,
                                              ID3DBlob **blob) {
  auto jobj = getJsonObj(path);
  const std::string &name = getFileName(path);
  return flatTablesRS(jobj, name, blob);
}
RootCompilerResult processSignatureFileToBlob(
    const char *path, ID3DBlob **blob, graphics::MaterialMetadata *metadata) {
  const std::string &name = getFileName(path);
  // bool useStaticSampler = shouldBindSamplers(jobj);
  bool useStaticSampler = true;

  // checking if we have pass definition, if not we shift the indeces
  bool hasPassConfig = metadata->passResourceCount != 0;
  bool hasConfig = metadata->objectResourceCount != 0;

  // we have one flat descriptor table to bind
  uint32_t registerCount = 1;
  registerCount += hasPassConfig ? 1 : 0;
  registerCount += hasConfig ? 1 : 0;
  registerCount += useStaticSampler ? 1 : 0;
  std::vector<CD3DX12_ROOT_PARAMETER> rootParams(registerCount);
  CD3DX12_DESCRIPTOR_RANGE ranges{};

  // create constant buffer for camera values
  int startRegister = 0;
  ranges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, startRegister,
              ENGINE_RESIGSTER_SPACE);
  rootParams[0].InitAsDescriptorTable(1, &ranges);

  // this is per object config
  int configIndex = 0 + useStaticSampler + static_cast<int>(hasPassConfig) +
                    static_cast<int>(hasConfig);

  int userCounter = 0;
  CD3DX12_DESCRIPTOR_RANGE *userRanges = nullptr;
  if (hasConfig) {
    userRanges = new CD3DX12_DESCRIPTOR_RANGE[metadata->objectResourceCount]{};
    for (int i = 0; i < metadata->objectResourceCount; ++i) {
      const auto &subConfig = metadata->objectResources[i];
      processRootConfiguration(&userRanges[userCounter], subConfig,
                               PSOManager::PER_OBJECT_BINDING_INDEX);
      ++userCounter;
    }
    rootParams[configIndex].InitAsDescriptorTable(userCounter, userRanges);
  }
  CD3DX12_DESCRIPTOR_RANGE *passRanges = nullptr;
  if (hasPassConfig) {
    passRanges = new CD3DX12_DESCRIPTOR_RANGE[metadata->passResourceCount]{};
    int passCounter = 0;
    for (int i = 0; i < metadata->passResourceCount; ++i) {
      const auto &subConfig = metadata->passResources[i];
      processRootConfiguration(&passRanges[passCounter], subConfig,
                               PSOManager::PER_PASS_BINDING_INDEX);
      ++passCounter;
    }
    // Magic number here is one because if we have a per pass index the
    // descriptor will be 1, zero will be the per frame data
    rootParams[1ll + useStaticSampler].InitAsDescriptorTable(passCounter,
                                                             passRanges);
  }

  UINT numStaticSampers = 0;
  D3D12_STATIC_SAMPLER_DESC const *staticSamplers = nullptr;

  CD3DX12_DESCRIPTOR_RANGE normalSamplersDesc;
  if (useStaticSampler) {
    auto normalSamplers = getSamplers();
    int samplersCount = static_cast<int>(getSamplersCount());
    int baseRegiser = 0;
    int space = 1;
    normalSamplersDesc.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, samplersCount,
                            baseRegiser, space);

    rootParams[1].InitAsDescriptorTable(1, &normalSamplersDesc);
  }

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
      static_cast<UINT>(rootParams.size()), rootParams.data(), numStaticSampers,
      staticSamplers);

  (*blob) = serializeRootSignature(rootSignatureDesc);

  return RootCompilerResult{
      frameString(name.c_str()),
      nullptr,
      ROOT_TYPE::NULL_TYPE,  // TODO need to see if this is needed, the pso
                             // should know about it
      true,
      static_cast<uint16_t>(userCounter),
      {0, useStaticSampler ? 1 : -1,
       static_cast<int16_t>(hasPassConfig ? 1 + useStaticSampler : -1),
       static_cast<int16_t>(useStaticSampler + hasPassConfig + 1)}};
}

RootCompilerResult processSignatureFile2(const char *path,
                                         graphics::MaterialMetadata *metadata) {
  ID3D12RootSignature *rootSig;
  ID3DBlob *blob;
  RootCompilerResult compilerResult =
      processSignatureFileToBlob(path, &blob, metadata);
  const HRESULT res = SirEngine::dx12::DEVICE->CreateRootSignature(
      1, blob->GetBufferPointer(), blob->GetBufferSize(),
      IID_PPV_ARGS(&(rootSig)));
  assert(res == S_OK);
  blob->Release();
  compilerResult.root = rootSig;
  return compilerResult;
}
}  // namespace SirEngine::dx12
