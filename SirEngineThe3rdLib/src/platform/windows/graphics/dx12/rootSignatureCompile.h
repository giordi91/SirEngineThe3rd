#pragma once
#include <d3d12.h>

#include <array>

#include "SirEngine/core.h"
#include "d3dx12.h"
#include "stdint.h"

namespace SirEngine::graphics {
struct MaterialMetadata;
}

namespace SirEngine::dx12 {
enum class ROOT_TYPE { RASTER = 0, COMPUTE = 1, DXR = 2, NULL_TYPE };
struct RootCompilerResult {
  const char *name;
  ID3D12RootSignature *root;
  ROOT_TYPE type;
  // 16 bits mostly for aligment
  uint16_t flatRoot;
  uint16_t descriptorCount;
  int16_t bindingSlots[4] = {-1, -1, -1, -1};
};
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> getStaticSamplers();
std::array<const D3D12_SAMPLER_DESC, 7> getSamplers();

RootCompilerResult SIR_ENGINE_API processSignatureFileToBlob(const char *path,
                                                             ID3DBlob **blob);
RootCompilerResult processSignatureFile(const char *path);
RootCompilerResult processSignatureFile2(const char *path,
                                         graphics::MaterialMetadata *metadata);
}  // namespace SirEngine::dx12
