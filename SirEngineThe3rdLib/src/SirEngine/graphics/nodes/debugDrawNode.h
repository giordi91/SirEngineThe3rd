
#pragma once

#include "SirEngine/graphics/nodeGraph.h"

namespace SirEngine {
class DebugDrawNode final : public GraphNode {
public:
  explicit DebugDrawNode(const char *name);
  virtual ~DebugDrawNode() = default;
  void initialize() override;
  void compute() override;

};

} // namespace SirEngine
