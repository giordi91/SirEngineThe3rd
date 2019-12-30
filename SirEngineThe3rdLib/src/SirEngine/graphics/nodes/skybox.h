#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "platform/windows/graphics/dx12/dx12MeshManager.h"
#include <d3d12.h>

namespace SirEngine {

class SkyBoxPass : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    DEPTH = INPUT_PLUG_CODE(1),
    OUT_TEX = OUTPUT_PLUG_CODE(0),
    COUNT = 3
  };

public:
  SkyBoxPass(GraphAllocators &allocators);
  virtual ~SkyBoxPass() = default;
  virtual void initialize() override;
  virtual void compute() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

  void populateNodePorts() override;
private:
  ID3D12RootSignature *rs = nullptr;
  PSOHandle pso;
  //handles
  MeshHandle skyboxHandle{};
  TextureHandle inputRTHandle{};
  TextureHandle inputDepthHandle{}; 
};

} // namespace SirEngine
