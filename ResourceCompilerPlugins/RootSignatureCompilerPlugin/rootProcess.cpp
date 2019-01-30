#include "rootProcess.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include <cassert>
#include <string>
#include <unordered_map>

enum class ROOT_TYPES { CONSTANT, DESCRIPTOR_TABLE, UAV, SRV, CBV, NULL_TYPE };

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

const std::unordered_map<std::string, ROOT_TYPES> STRING_TO_ROOT_TYPE{
    {"constant", ROOT_TYPES::CONSTANT},
    {"descriptorTable", ROOT_TYPES::DESCRIPTOR_TABLE},
    {"UAV", ROOT_TYPES::UAV},
    {"SRV", ROOT_TYPES::SRV},
    {"CBV", ROOT_TYPES::CBV}};

const std::unordered_map<std::string, SirEngine::dx12::ROOT_FILE_TYPE>
    STRING_TO_ROOT_FILE_TYPE{
        {"RASTER", SirEngine::dx12::ROOT_FILE_TYPE::RASTER},
        {"COMPUTE", SirEngine::dx12::ROOT_FILE_TYPE::COMPUTE},
        {"DXR", SirEngine::dx12::ROOT_FILE_TYPE::DXR},
    };

const std::unordered_map<std::string, D3D12_SHADER_VISIBILITY>
    STRING_TO_VISIBILITY_FLAG{{"ALL", D3D12_SHADER_VISIBILITY_ALL},
                              {"VERTEX", D3D12_SHADER_VISIBILITY_VERTEX},
                              {"HULL", D3D12_SHADER_VISIBILITY_HULL},
                              {"DOMAIN", D3D12_SHADER_VISIBILITY_DOMAIN},
                              {"GEOMETRY", D3D12_SHADER_VISIBILITY_GEOMETRY},
                              {"PIXEL", D3D12_SHADER_VISIBILITY_PIXEL}};

ROOT_TYPES
getTypeEnum(const std::string &type) {
  auto found = STRING_TO_ROOT_TYPE.find(type);
  if (found != STRING_TO_ROOT_TYPE.end()) {
    return found->second;
  }
  assert(0 && "could not convert root type from string to enum");
  return ROOT_TYPES::NULL_TYPE;
}
SirEngine::dx12::ROOT_FILE_TYPE getFileTypeEnum(const std::string &type) {
  auto found = STRING_TO_ROOT_FILE_TYPE.find(type);
  if (found != STRING_TO_ROOT_FILE_TYPE.end()) {
    return found->second;
  }
  assert(0 && "could not convert root type from string to enum");
  return SirEngine::dx12::ROOT_FILE_TYPE::NULL_TYPE;
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
      SE_CORE_WARN("Cannot convert shader visibility from string, value not "
                   "recognized: {0}",
                   visString);
      SE_CORE_WARN("D3D12_SHADER_VISIBILITY_ALL will be used");
    }
  }
  return toReturn;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> getStaticSamplers() {
  // Applications usually only need a handful of samplers.  So just define them
  // all up front and keep them available as part of the root signature.

  const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
      0,                                // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

  const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
      1,                                 // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

  const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
      2,                                // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_LINEAR,  // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

  const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
      3,                                 // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

  const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
      4,                               // shaderRegister
      D3D12_FILTER_ANISOTROPIC,        // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressW
      0.0f,                            // mipLODBias
      8);                              // maxAnisotropy

  const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
      5,                                // shaderRegister
      D3D12_FILTER_ANISOTROPIC,         // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressW
      0.0f,                             // mipLODBias
      8);                               // maxAnisotropy

  return {pointWrap,   pointClamp,      linearWrap,
          linearClamp, anisotropicWrap, anisotropicClamp};
}

void processSRV(nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param) {
  int defaultInt = -1;
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto &jdata = jobj[ROOT_KEY_DATA];

  int startRegister =
      getValueIfInJson(jdata, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  param.InitAsShaderResourceView(startRegister, 0, getVisibility(jobj));
}
void processCBV(nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param) {
  int defaultInt = -1;
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto &jdata = jobj[ROOT_KEY_DATA];

  int startRegister =
      getValueIfInJson(jdata, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  param.InitAsConstantBufferView(startRegister, 0, getVisibility(jobj));
}

void processConstant(nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param) {
  int defaultInt = -1;
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto &jdata = jobj[ROOT_KEY_DATA];

  int sizeIn32Bit =
      getValueIfInJson(jdata, ROOT_KEY_SIZE_IN_32_BIT_VALUES, defaultInt);
  assert(sizeIn32Bit != -1);
  int startRegister =
      getValueIfInJson(jdata, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  param.InitAsConstants(sizeIn32Bit, startRegister, 0, getVisibility(jobj));
}

void initDescriptorAsUAV(nlohmann::json &jobj,
                         CD3DX12_DESCRIPTOR_RANGE &descriptor) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, count, startRegister);
}

void initDescriptorAsSRV(nlohmann::json &jobj,
                         CD3DX12_DESCRIPTOR_RANGE &descriptor) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, count, startRegister);
}

void initDescriptorAsConstant(nlohmann::json &jobj,
                              CD3DX12_DESCRIPTOR_RANGE &descriptor) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, count, startRegister);
}

void processDescriptorTable(nlohmann::json &jobj,
                            CD3DX12_ROOT_PARAMETER &param) {
  size_t size = jobj.size();
  assert(size != 0);
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto dataJ = jobj[ROOT_KEY_DATA];
  assert(dataJ.find(ROOT_KEY_RANGES) != dataJ.end());
  auto rangesJ = dataJ[ROOT_KEY_RANGES];
  auto *ranges = new CD3DX12_DESCRIPTOR_RANGE[rangesJ.size()];
  int counter = 0;
  const std::string defaultString = "";
  for (auto &subRange : rangesJ) {

    std::string typeString =
        getValueIfInJson(subRange, ROOT_KEY_TYPE, defaultString);
    ROOT_TYPES descriptorType = getTypeEnum(typeString);
    switch (descriptorType) {
    case (ROOT_TYPES::UAV): {
      initDescriptorAsUAV(subRange, ranges[counter]);
      break;
    }
    case (ROOT_TYPES::SRV): {
      initDescriptorAsSRV(subRange, ranges[counter]);
      break;
    }
    case (ROOT_TYPES::CBV): {
      initDescriptorAsConstant(subRange, ranges[counter]);
      break;
    }
    default: {
    }
    }
    ++counter;
  }
  param.InitAsDescriptorTable(counter, ranges);
}
ID3DBlob *
serializeRootSignature(D3D12_ROOT_SIGNATURE_DESC &desc) {
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

void processSignatureFile(const char *path, std::vector<ResultRoot> &blobs) {
  auto jobj = get_json_obj(path);
  // there might be multiple signatures in the files lets loop them

  const std::string defaultString;
  for (auto it = jobj.begin(); it != jobj.end(); ++it) {
    const std::string &name = it.key();

    auto jvalue = it.value();

    std::string fileType =
        getValueIfInJson(jvalue, ROOT_KEY_TYPE, defaultString);

    assert(!fileType.empty());

    assert(jvalue.find(ROOT_KEY_CONFIG) != jvalue.end());
    auto &configValue = jvalue[ROOT_KEY_CONFIG];

    std::vector<CD3DX12_ROOT_PARAMETER> rootParams(configValue.size());
    int counter = 0;
    for (auto &subConfig : configValue) {
      // getting the type of root
      std::string configTypeValueString =
          getValueIfInJson(subConfig, ROOT_KEY_TYPE, defaultString);
      assert(!configTypeValueString.empty());
      ROOT_TYPES configType = getTypeEnum(configTypeValueString);
      switch (configType) {
      case (ROOT_TYPES::CONSTANT): {
        processConstant(subConfig, rootParams[counter]);
        break;
      }
      case (ROOT_TYPES::DESCRIPTOR_TABLE): {
        processDescriptorTable(subConfig, rootParams[counter]);
        break;
      }
      case (ROOT_TYPES::SRV): {
        processSRV(subConfig, rootParams[counter]);
        break;
      }
      case (ROOT_TYPES::CBV): {
        processCBV(subConfig, rootParams[counter]);
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

    if (shouldBindSamplers(jvalue)) {
      staticSamplers = &samplers[0];
      numStaticSampers = static_cast<UINT>(samplers.size());
    }

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
        static_cast<UINT>(rootParams.size()), rootParams.data(), numStaticSampers, staticSamplers);

    // process flags
    auto found = jvalue.find(ROOT_KEY_FLAGS);
    if (found != jvalue.end()) {
      auto &jflags = jvalue[ROOT_KEY_FLAGS];
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

    SirEngine::dx12::ROOT_FILE_TYPE fileTypeEnum = getFileTypeEnum(fileType);
    ID3DBlob *blob =
        serializeRootSignature(rootSignatureDesc);
    blobs.emplace_back(ResultRoot{name, blob, fileTypeEnum});
  }
}
