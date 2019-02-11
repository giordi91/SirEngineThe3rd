#pragma once

#include "SirEngine/graphics/nodeGraph.h"

namespace SirEngine {
class SimpleForward : public GraphNode {
public:
  SimpleForward(const char *name);
  virtual ~SimpleForward() = default;
  virtual void compute() override;
};

} // namespace SirEngine
