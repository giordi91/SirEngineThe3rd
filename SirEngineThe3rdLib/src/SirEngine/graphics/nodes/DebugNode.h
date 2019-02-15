#pragma once

#include "SirEngine/graphics/nodeGraph.h"

namespace SirEngine {
class DebugNode final : public GraphNode {
public:
  DebugNode(const char* name);
  virtual ~DebugNode() = default;
  virtual void compute() override;
};

} // namespace SirEngine
