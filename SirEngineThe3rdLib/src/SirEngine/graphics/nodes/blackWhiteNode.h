#pragma once

#include "SirEngine/graphics/nodeGraph.h"

namespace SirEngine {
class BlackWhiteNode final : public GraphNode {
public:
  BlackWhiteNode(const char* name);
  virtual ~BlackWhiteNode() = default;
  virtual void compute() override;
};

} // namespace SirEngine
