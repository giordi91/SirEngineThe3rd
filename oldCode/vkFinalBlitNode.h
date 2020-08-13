#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {
class VkFinalBlitNode final : public GNode {
public:
  enum PLUGS { IN_TEXTURE = inputPlugCode(0), COUNT = 1 };
public:
  VkFinalBlitNode(GraphAllocators &allocators);
  virtual ~VkFinalBlitNode() = default;
  virtual void compute() override;
  virtual void initialize() override;

  void populateNodePorts() override;
private:
};

} // namespace SirEngine
