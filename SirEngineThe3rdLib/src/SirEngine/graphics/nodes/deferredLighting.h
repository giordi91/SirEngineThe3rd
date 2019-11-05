#pragma once

#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include <d3d12.h>

namespace SirEngine {

class DeferredLightingPass : public GNode {
public:
  enum PLUGS {
    GEOMETRY_RT = INPUT_PLUG_CODE(0),
    NORMALS_RT = INPUT_PLUG_CODE(1),
    SPECULAR_RT = INPUT_PLUG_CODE(2),
    DEPTH_RT = INPUT_PLUG_CODE(3),
    DIRECTIONAL_SHADOW_RT = INPUT_PLUG_CODE(4),
    LIGHTING_RT = OUTPUT_PLUG_CODE(0),
    COUNT = 6
  };

public:
  DeferredLightingPass(GraphAllocators &allocators);
  virtual ~DeferredLightingPass() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void clear() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

private:
  TextureHandle m_lightBuffer{};
  TextureHandle m_brdfHandle{};
  ID3D12RootSignature *rs = nullptr;
  PSOHandle pso;
  ConstantBufferHandle m_lightCB;
  D3D12_GPU_VIRTUAL_ADDRESS m_lightAddress;
};

} // namespace SirEngine
