#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {

	class SimpleForward : public GraphNode {
public:
  SimpleForward(const char *name);
  virtual ~SimpleForward() = default;
  virtual void initialize() override;
  virtual void compute() override;
private:
	TextureHandle m_renderTarget;
	TextureHandle m_depth;
};

} // namespace SirEngine
