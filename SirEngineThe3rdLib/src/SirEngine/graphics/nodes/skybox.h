#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "platform/windows/graphics/dx12/dx12MeshManager.h"
//#include <d3d12.h>

namespace SirEngine {

class SkyBoxPass : public GNode {
public:
  enum PLUGS :uint32_t{
    IN_TEXTURE = inputPlugCode(0),
    DEPTH = inputPlugCode(1),
    OUT_TEX = outputPlugCode(0),
    COUNT = 3
  };

public:
  SkyBoxPass(GraphAllocators &allocators);
  virtual ~SkyBoxPass() = default;
  virtual void initialize() override;
  virtual void compute() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

  void populateNodePorts() override;
  void clear() override;
private:
  //ID3D12RootSignature *rs = nullptr;
  //PSOHandle pso;
  //handles
  TextureHandle inputRTHandle{};
  TextureHandle inputDepthHandle{}; 
  BufferBindingsHandle m_bindHandle{};
  RSHandle m_rs{};
  PSOHandle m_pso{};
  BindingTableHandle m_bindingTable{};
};

} // namespace SirEngine
