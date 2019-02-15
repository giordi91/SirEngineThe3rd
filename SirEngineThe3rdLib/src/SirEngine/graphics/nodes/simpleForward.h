#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {

class SimpleForward : public GraphNode {
public:
  SimpleForward(const char *name);
  virtual ~SimpleForward() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void clear() override;

private:
  TextureHandle m_renderTarget{};
  TextureHandle m_depth{};
};

} // namespace SirEngine
