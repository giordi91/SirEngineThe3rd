#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {
class FinalBlitNode final : public GNode {
public:
  enum PLUGS { IN_TEXTURE = inputPlugCode(0), COUNT = 1 };
public:
  explicit FinalBlitNode(GraphAllocators &allocators);
  virtual ~FinalBlitNode() = default;
  virtual void compute() override;
  virtual void initialize(CommandBufferHandle commandBuffer) override;
  void populateNodePorts() override;
  void clear() override;
  void clearResolutionDepenantResources() override;
  void onResizeEvent(int, int, CommandBufferHandle commandBuffer) override;
private:
  TextureHandle inputRTHandle;
  BufferBindingsHandle m_bindHandle{};
  BindingTableHandle m_bindingTable{};
  PSOHandle m_pso;
  RSHandle m_rs;
};

} // namespace SirEngine
