#pragma once
#include "SirEngine/core.h"
#include <d3d12.h>

namespace SirEngine::dx12 {
enum class PSOType { DXR = 0, RASTER, COMPUTE, INVALID };
struct SIR_ENGINE_API PSOCompileResult {
  D3D12_COMPUTE_PIPELINE_STATE_DESC *computeDesc = nullptr;
  D3D12_GRAPHICS_PIPELINE_STATE_DESC *graphicDesc = nullptr;
  ID3D12PipelineState *pso = nullptr;
  PSOType psoType = PSOType::INVALID;
  const char *VSName = nullptr;
  const char *PSName = nullptr;
  const char *CSName = nullptr;
  //TODO remove PSO name, useless, can be recovered from FullPath
  const char *PSOName = nullptr;
  const char *PSOFullPathFile = nullptr;
  const char *inputLayout = nullptr;
  const char *rootSignature = nullptr;
};

//loads a PSO json definition and compiles everything from scratch
//this means the PSO, the Root signature and shaders
PSOCompileResult SIR_ENGINE_API compileRawPSO(const char *path, const char* shaderPath);
PSOType convertStringPSOTypeToEnum(const char *type);

} // namespace SirEngine::dx12
