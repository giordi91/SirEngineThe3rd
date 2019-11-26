#pragma once
#include <d3d12.h>
#include "SirEngine/core.h"

namespace SirEngine::dx12 {
enum class ROOT_TYPE { RASTER = 0, COMPUTE = 1, DXR = 2, NULL_TYPE };
struct RootCompilerResult {
  const char *name;
  ID3D12RootSignature *root;
  ROOT_TYPE type;
};

RootCompilerResult SIR_ENGINE_API processSignatureFileToBlob(const char *path,ID3DBlob ** blob);
RootCompilerResult processSignatureFile(const char *path);
} // namespace SirEngine::dx12
