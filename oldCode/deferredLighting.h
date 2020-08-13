#pragma once

#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include <d3d12.h>

namespace SirEngine {

class DeferredLightingPass : public GNode {
public:
  enum PLUGS :uint32_t{
    GEOMETRY_RT = inputPlugCode(0),
    NORMALS_RT = inputPlugCode(1),
    SPECULAR_RT = inputPlugCode(2),
    DEPTH_RT = inputPlugCode(3),
    DIRECTIONAL_SHADOW_RT = inputPlugCode(4),
    LIGHTING_RT = outputPlugCode(0),
    COUNT = 6
  };

public:
  DeferredLightingPass(GraphAllocators &allocators);
  virtual ~DeferredLightingPass() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void clear() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

  void populateNodePorts() override;

private:
  TextureHandle m_lightBuffer{};
  TextureHandle m_brdfHandle{};
  ID3D12RootSignature *rs = nullptr;
  PSOHandle pso;
  ConstantBufferHandle m_lightCB;
  D3D12_GPU_VIRTUAL_ADDRESS m_lightAddress;

  // handles
  TextureHandle gbufferHandle;
  TextureHandle normalBufferHandle;
  TextureHandle specularBufferHandle;
  TextureHandle depthHandle;
};

} // namespace SirEngine
