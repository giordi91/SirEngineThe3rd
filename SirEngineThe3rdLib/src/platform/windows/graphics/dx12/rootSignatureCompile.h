#pragma once
#include <d3d12.h>

namespace SirEngine::dx12 {
enum class ROOT_TYPE { RASTER = 0, COMPUTE = 1, DXR = 2, NULL_TYPE };
struct RootCompilerResult {
  const char *name;
  ID3D12RootSignature *root;
  ROOT_TYPE type;
};

RootCompilerResult processSignatureFile(const char *path);
} // namespace SirEngine::dx12
