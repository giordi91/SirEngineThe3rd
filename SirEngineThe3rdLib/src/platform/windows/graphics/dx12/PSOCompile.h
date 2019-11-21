#pragma once
#include <d3d12.h>

namespace SirEngine::dx12 {
enum class PSOType { DXR = 0, RASTER, COMPUTE, INVALID };
struct PSOCompileResult {
  ID3D12PipelineState *pso;
  PSOType psoType;
  const char *VSName = nullptr;
  const char *PSName = nullptr;
  const char *ComputeName = nullptr;
  const char *PSOName;
  const char *PSOFullPathFile;
};
PSOCompileResult loadPSOFile(const char *path);
PSOType convertStringPSOTypeToEnum(const char*type); 

} // namespace SirEngine::dx12
