#pragma once
#include <d3d12.h>

#include "SirEngine/core.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/handle.h"
#include "SirEngine/materialManager.h"

namespace SirEngine::dx12 {
struct SIR_ENGINE_API PSOCompileResult {
  D3D12_COMPUTE_PIPELINE_STATE_DESC *computeDesc = nullptr;
  D3D12_GRAPHICS_PIPELINE_STATE_DESC *graphicDesc = nullptr;
  ID3D12PipelineState *pso = nullptr;
  RSHandle rsHandle = {};
  PSO_TYPE psoType = PSO_TYPE::INVALID;
  const char *VSName = nullptr;
  const char *PSName = nullptr;
  const char *CSName = nullptr;
  const char *PSOFullPathFile = nullptr;
  const char *inputLayout = nullptr;
  const char *rootSignature = nullptr;
  TOPOLOGY_TYPE topologyType;
  MaterialMetadata metadata;
};

// loads a PSO json definition and compiles everything from scratch
// this means the PSO, the Root signature and shaders
PSOCompileResult SIR_ENGINE_API compileRawPSO(const char *path,
                                              const char *shaderPath);
PSO_TYPE convertStringPSOTypeToEnum(const char *type);

}  // namespace SirEngine::dx12
