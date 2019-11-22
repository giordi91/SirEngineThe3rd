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
  const char *ComputeName = nullptr;
  const char *PSOName = nullptr;
  const char *PSOFullPathFile = nullptr;
  const char *inputLayout = nullptr;
  const char *rootSignature = nullptr;
};
PSOCompileResult SIR_ENGINE_API loadPSOFile(const char *path);
PSOType convertStringPSOTypeToEnum(const char *type);

} // namespace SirEngine::dx12
