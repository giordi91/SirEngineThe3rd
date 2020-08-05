
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
  void initialize() override;
  void compute() override;

  void populateNodePorts() override;

  void clear() override;

 private:
  TextureHandle inputRTHandle{};
  TextureHandle inputDepthHandle{};
  BufferBindingsHandle m_bindHandle{};
};

}  // namespace SirEngine
