#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include <d3d12.h>

namespace SirEngine {

class ProceduralSkyBoxPass : public GNode {
public:
  enum PLUGS {
    FULLSCREEN_PASS= INPUT_PLUG_CODE(0),
    DEPTH_RT = INPUT_PLUG_CODE(1),
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
    COUNT = 3
  };
public:
  explicit ProceduralSkyBoxPass(GraphAllocators &allocators);
  virtual ~ProceduralSkyBoxPass()=default; 
  virtual void initialize() override;
  virtual void compute() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

  void populateNodePorts() override;
private:
  TextureHandle m_skyboxBuffer{};
  ID3D12RootSignature *rs = nullptr;
  PSOHandle pso;
  //handles
  TextureHandle inputRTHandle{};
  TextureHandle inputDepthHandle{};
};

} // namespace SirEngine
