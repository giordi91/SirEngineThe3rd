
#pragma once

#include "SirEngine/graphics/nodeGraph.h"

namespace SirEngine {
class DebugDrawNode final : public GNode {
 public:
  enum PLUGS : uint32_t {
    IN_TEXTURE = inputPlugCode(0),
    DEPTH_RT = inputPlugCode(1),
    OUT_TEXTURE = outputPlugCode(0),
    COUNT = 2
  };

 public:
  explicit DebugDrawNode(GraphAllocators &allocators);
  virtual ~DebugDrawNode() = default;
  void initialize(CommandBufferHandle commandBuffer,RenderGraphContext* context) override;
  void compute() override;

  void populateNodePorts(RenderGraphContext* context) override;

  void clear() override;

  void clearResolutionDepenantResources() override;
  void onResizeEvent(int, int, CommandBufferHandle commandBuffer,RenderGraphContext* context) override;
 private:
  TextureHandle inputRTHandle{};
  TextureHandle inputDepthHandle{};
  BufferBindingsHandle m_bindHandle{};
};

}  // namespace SirEngine
