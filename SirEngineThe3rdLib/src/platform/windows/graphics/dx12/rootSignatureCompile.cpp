#include "platform/windows/graphics/dx12/rootSignatureCompile.h"

#include <assert.h>

#include "SirEngine/io/fileUtils.h"
#include "SirEngine/graphics/materialMetadata.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"
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

static const uint32_t ENGINE_RESIGSTER_SPACE = 0;

SUB_ROOT_TYPES
getTypeEnum(const graphics::MATERIAL_RESOURCE_TYPE type,
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
              (static_cast<uint32_t>(
                   graphics::MATERIAL_RESOURCE_FLAGS::READ_ONLY) > 0))
                 ? SUB_ROOT_TYPES::SRV
                 : SUB_ROOT_TYPES::UAV;
    }
    default: {
      assert(0 && "descriptor type not suppoerted");
      return SUB_ROOT_TYPES::NULL_TYPE;
    }
  }
}

static D3D12_SAMPLER_DESC STATIC_SAMPLERS_DESCS[STATIC_SAMPLERS_COUNT] = {};

const D3D12_SAMPLER_DESC *getSamplers() {
  // Applications usually only need a handful of samplers.  So just define them
  // all up front and keep them available as part of the root signature.

  const D3D12_SAMPLER_DESC pointWrap{
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

  STATIC_SAMPLERS_DESCS[0] = pointWrap;
  STATIC_SAMPLERS_DESCS[1] = pointClamp;
  STATIC_SAMPLERS_DESCS[2] = linearWrap;
  STATIC_SAMPLERS_DESCS[3] = linearClamp;
  STATIC_SAMPLERS_DESCS[4] = anisotropicWrap;
  STATIC_SAMPLERS_DESCS[5] = anisotropicClamp;
  STATIC_SAMPLERS_DESCS[6] = shadowPCFClamp;
  return STATIC_SAMPLERS_DESCS;
}

void initDescriptorAsUAV(const graphics::MaterialResource &metadata,
                         CD3DX12_DESCRIPTOR_RANGE &descriptor) {
  int startRegister = metadata.binding;
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, startRegister,
                  metadata.set);
}

void initDescriptorAsSRV(const graphics::MaterialResource &metadata,
                         CD3DX12_DESCRIPTOR_RANGE &descriptor) {
  int startRegister = metadata.binding;
  assert(startRegister != -1);
  descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, startRegister,
                  metadata.set);
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

void processRootConfiguration(CD3DX12_DESCRIPTOR_RANGE *userRanges,
                              const graphics::MaterialResource &metadata) {
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
    for (uint32_t i = 0; i < metadata->objectResourceCount; ++i) {
      const auto &subConfig = metadata->objectResources[i];
      processRootConfiguration(&userRanges[userCounter], subConfig);
      ++userCounter;
    }
    rootParams[configIndex].InitAsDescriptorTable(userCounter, userRanges);
  }
  CD3DX12_DESCRIPTOR_RANGE *passRanges = nullptr;
  if (hasPassConfig) {
    passRanges = new CD3DX12_DESCRIPTOR_RANGE[metadata->passResourceCount]{};
    int passCounter = 0;
    for (uint32_t i = 0; i < metadata->passResourceCount; ++i) {
      const auto &subConfig = metadata->passResources[i];
      processRootConfiguration(&passRanges[passCounter], subConfig);
      ++passCounter;
    }
    // Magic number here is one because if we have a per pass index the
    // descriptor will be 1, zero will be the per frame data
    rootParams[1ll + useStaticSampler].InitAsDescriptorTable(passCounter,
                                                             passRanges);
  }

  // static samplers are always used
  CD3DX12_DESCRIPTOR_RANGE normalSamplersDesc;
  int samplersCount = static_cast<int>(STATIC_SAMPLERS_COUNT);
  int baseRegiser = 0;
  int space = 1;
  normalSamplersDesc.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, samplersCount,
                          baseRegiser, space);

  rootParams[1].InitAsDescriptorTable(1, &normalSamplersDesc);

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
      static_cast<UINT>(rootParams.size()), rootParams.data(), 0, nullptr);

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

RootCompilerResult processSignatureFile(const char *path,
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
