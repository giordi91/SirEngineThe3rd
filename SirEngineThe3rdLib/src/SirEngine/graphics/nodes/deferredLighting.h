#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include <d3d12.h>
#include "SirEngine/graphics/cpuGraphicsStructures.h"

namespace SirEngine {

class DeferredLightingPass : public GraphNode {
public:
  DeferredLightingPass(const char *name);
  virtual ~DeferredLightingPass() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void clear() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

private:
  TextureHandle m_lightBuffer{};
  TextureHandle m_brdfHandle{};
  ID3D12RootSignature* rs =nullptr;
  PSOHandle pso;
  DirectionalLightData m_light;
  ConstantBufferHandle m_lightCB;
  D3D12_GPU_VIRTUAL_ADDRESS m_lightAddress;
};

} // namespace SirEngine
