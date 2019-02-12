#pragma once

#include "SirEngine/graphics/nodeGraph.h"

namespace SirEngine {
class FinalBlitNode final : public GraphNode {
public:
  FinalBlitNode();
  virtual ~FinalBlitNode() = default;
  virtual void compute()override;
};

} // namespace SirEngine
