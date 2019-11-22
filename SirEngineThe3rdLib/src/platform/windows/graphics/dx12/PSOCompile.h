#pragma once
#include <d3d12.h>
#include "SirEngine/core.h"

namespace SirEngine::dx12 {
enum class PSOType { DXR = 0, RASTER, COMPUTE, INVALID };
struct SIR_ENGINE_API PSOCompileResult {
  ID3D12PipelineState *pso;
  PSOType psoType;
  const char *VSName = nullptr;
  const char *PSName = nullptr;
  const char *ComputeName = nullptr;
  const char *PSOName;
  const char *PSOFullPathFile;
};
PSOCompileResult SIR_ENGINE_API loadPSOFile(const char *path);
PSOType convertStringPSOTypeToEnum(const char*type); 

} // namespace SirEngine::dx12
