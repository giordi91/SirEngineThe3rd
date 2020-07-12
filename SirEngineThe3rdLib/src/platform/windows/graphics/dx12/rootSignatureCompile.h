#pragma once
#include <d3d12.h>
#include "SirEngine/core.h"
#include "stdint.h"

namespace SirEngine::dx12 {
enum class ROOT_TYPE { RASTER = 0, COMPUTE = 1, DXR = 2, NULL_TYPE };
struct RootCompilerResult {
  const char *name;
  ID3D12RootSignature *root;
  ROOT_TYPE type;
  //16 bits mostly for aligment
  uint16_t flatRoot;
  uint16_t descriptorCount;
};

RootCompilerResult SIR_ENGINE_API processSignatureFileToBlob(const char *path,ID3DBlob ** blob);
RootCompilerResult processSignatureFile(const char *path);
ID3D12RootSignature *enginePerFrameEmptyRS(const char *name);
} // namespace SirEngine::dx12
