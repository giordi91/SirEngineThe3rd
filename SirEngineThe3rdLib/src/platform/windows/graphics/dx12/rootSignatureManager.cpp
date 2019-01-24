#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "SirEngine/fileUtils.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "nlohmann/json.hpp"
#include <d3d12.h>
#include <iostream>

namespace temp{
namespace rendering {

const std::string RootSignatureManager::ROOT_KEY_CONFIG = "config";
const std::string RootSignatureManager::ROOT_KEY_NAME = "name";
const std::string RootSignatureManager::ROOT_KEY_TYPE = "type";
const std::string RootSignatureManager::ROOT_KEY_DATA = "data";
const std::string RootSignatureManager::ROOT_KEY_NUM_DESCRIPTOR =
    "numDescriptors";
const std::string RootSignatureManager::ROOT_KEY_RANGES = "ranges";
const std::string RootSignatureManager::ROOT_KEY_REGISTER = "register";
const std::string RootSignatureManager::ROOT_KEY_BASE_REGISTER = "baseRegister";
const std::string RootSignatureManager::ROOT_KEY_SIZE_IN_32_BIT_VALUES =
    "sizeIn32BitValues";
const std::string RootSignatureManager::ROOT_KEY_VISIBILITY = "visibility";
const std::string RootSignatureManager::ROOT_KEY_FLAGS = "flags";
const std::string RootSignatureManager::ROOT_KEY_FLAGS_LOCAL = "local";
const std::string RootSignatureManager::ROOT_EMPTY = "empty";

const std::unordered_map<std::string, RootSignatureManager::ROOT_TYPES>
    RootSignatureManager::m_stringToRootType{
        {"constant", RootSignatureManager::ROOT_TYPES::CONSTANT},
        {"descriptorTable", RootSignatureManager::ROOT_TYPES::DESCRIPTOR_TABLE},
        {"UAV", RootSignatureManager::ROOT_TYPES::UAV},
        {"SRV", RootSignatureManager::ROOT_TYPES::SRV},
        {"CBV", RootSignatureManager::ROOT_TYPES::CBV}};

void RootSignatureManager::init(ID3D12Device *device) {
  m_device = device;
  // Nvidia drivers requires always a signature, this can be used as a null
  // signature
  createNullRootSignature();
}
void RootSignatureManager::cleanup() {
  // cleanup the allocated root signatures
  for (auto it : m_rootRegister) {
    it.second->Release();
  }
  m_rootRegister.clear();
}
void RootSignatureManager::loadSingaturesInFolder(const char *directory) {

  std::vector<std::string> paths;
  listFilesInFolder(directory, paths, "json");

  for (const auto &p : paths) {
    loadSignatureFile(p.c_str());
  }
}

void RootSignatureManager::processDescriptorTable(
    nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param) {
  int size = jobj.size();
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

void RootSignatureManager::processSRV(nlohmann::json &jobj,
                                      CD3DX12_ROOT_PARAMETER &param) {
  int defaultInt = -1;
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto &jdata = jobj[ROOT_KEY_DATA];

  int startRegister =
      getValueIfInJson(jdata, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  param.InitAsShaderResourceView(startRegister);
}
void RootSignatureManager::processCBV(nlohmann::json &jobj,
                                      CD3DX12_ROOT_PARAMETER &param) {
  int defaultInt = -1;
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto &jdata = jobj[ROOT_KEY_DATA];

  int startRegister =
      getValueIfInJson(jdata, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  param.InitAsConstantBufferView(startRegister);
}

void RootSignatureManager::processConstant(nlohmann::json &jobj,
                                           CD3DX12_ROOT_PARAMETER &param) {
  int defaultInt = -1;
  assert(jobj.find(ROOT_KEY_DATA) != jobj.end());
  auto &jdata = jobj[ROOT_KEY_DATA];

  int sizeIn32Bit =
      getValueIfInJson(jdata, ROOT_KEY_SIZE_IN_32_BIT_VALUES, defaultInt);
  assert(sizeIn32Bit != -1);
  int startRegister =
      getValueIfInJson(jdata, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  param.InitAsConstants(sizeIn32Bit, startRegister);
}

void RootSignatureManager::initDescriptorAsUAV(
    nlohmann::json &jobj, CD3DX12_DESCRIPTOR_RANGE &descriptor) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, count, startRegister);
}

void RootSignatureManager::initDescriptorAsSRV(
    nlohmann::json &jobj, CD3DX12_DESCRIPTOR_RANGE &descriptor) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, count, startRegister);
}

void RootSignatureManager::initDescriptorAsConstant(
    nlohmann::json &jobj, CD3DX12_DESCRIPTOR_RANGE &descriptor) {
  int defaultInt = -1;
  int count = getValueIfInJson(jobj, ROOT_KEY_NUM_DESCRIPTOR, defaultInt);
  assert(count != -1);
  int startRegister =
      getValueIfInJson(jobj, ROOT_KEY_BASE_REGISTER, defaultInt);
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, count, startRegister);
}

void RootSignatureManager::loadSignatureFile(const char *path) {
  auto &jobj = get_json_obj(path);
  // there might be multiple signatures in the files lets loop them
  std::cout << "[Engine]: Loading root signatures from: " << path << std::endl;

  const std::string defaultString = "";
  for (auto it = jobj.begin(); it != jobj.end(); ++it) {
    const std::string name = it.key();
    std::cout << " -----> " << name << std::endl;

    auto jvalue = it.value();
    assert(jvalue.find(ROOT_KEY_CONFIG) != jvalue.end());
    auto &configValue = jvalue[ROOT_KEY_CONFIG];

    std::vector<CD3DX12_ROOT_PARAMETER> rootParams(configValue.size());
    int counter = 0;
    for (auto &subConfig : configValue) {
      // getting the type of root
      std::string &configTypeValueString =
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

    // Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
    ID3D12RootSignature *rootSig;
    CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(rootParams.size(),
                                                        rootParams.data());

    // process flags
    auto found = jvalue.find(ROOT_KEY_FLAGS);
    if (found != jvalue.end()) {
      auto &jflags = jvalue[ROOT_KEY_FLAGS];
      for (auto &subFlag : jflags) {
        if (subFlag.get<std::string>() == ROOT_KEY_FLAGS_LOCAL) {
          globalRootSignatureDesc.Flags |=
              D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        }
		if (subFlag.get<std::string>() == "D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT")
		{
			globalRootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		}
      }
    }

    SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc,
                                              &rootSig);

    // register the root signature
    m_rootRegister[name] = rootSig;
  }
}
void RootSignatureManager::SerializeAndCreateRaytracingRootSignature(
    D3D12_ROOT_SIGNATURE_DESC &desc, ID3D12RootSignature **rootSig) {
  ID3DBlob *blob;
  ID3DBlob *error;
  {
    HRESULT res = D3D12SerializeRootSignature(
        &desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
  if (error!= nullptr) {
    std::cout << ((char *)error->GetBufferPointer()) << std::endl;
  }
    assert(res == S_OK);
    //	  , error ?
    //  static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
    res = m_device->CreateRootSignature(1, blob->GetBufferPointer(),
                                        blob->GetBufferSize(), // rootSig);
                                        IID_PPV_ARGS(&(*rootSig)));
    assert(res == S_OK);
    blob->Release();
    if (error != nullptr) {
      error->Release();
    }
  }
}

void RootSignatureManager::createNullRootSignature() {
	return;
  ID3D12RootSignature *rootSig;
  CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(D3D12_DEFAULT);
  localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
  SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &rootSig);
  //m_rootRegister[ROOT_EMPTY] = rootSig;
}

RootSignatureManager::ROOT_TYPES
RootSignatureManager::getTypeEnum(const std::string &type) {
  auto found = m_stringToRootType.find(type);
  if (found != m_stringToRootType.end()) {
    return found->second;
  } else {
    assert(0 && "could not convert root type from string to enum");
    return ROOT_TYPES::NULL_TYPE;
  }

  return ROOT_TYPES();
}
} // namespace rendering
} // namespace dx12
