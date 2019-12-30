
#pragma once

#include "SirEngine/graphics/nodeGraph.h"

namespace SirEngine {
class DebugDrawNode final : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    DEPTH_RT = INPUT_PLUG_CODE(1),
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
    COUNT = 2
  };

public:
  explicit DebugDrawNode(GraphAllocators &allocators);
  virtual ~DebugDrawNode() = default;
  void initialize() override;
  void compute() override;

  void populateNodePorts() override;

private:
  TextureHandle inputRTHandle{};
  TextureHandle inputDepthHandle{};
};

} // namespace SirEngine
