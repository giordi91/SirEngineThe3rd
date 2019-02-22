#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {
class DebugNode final : public GraphNode {
public:
  DebugNode(const char *name);
  virtual ~DebugNode() = default;
  virtual void compute() override;
  void setDebugIndex(int index) { m_index = static_cast<DebugIndex>(index); }

private:
  enum DebugIndex {
    GBUFFER = 1,
    NORMAL_BUFFER,
    SPECULAR_BUFFER,
    GBUFFER_DEPTH,
  };

private:
  void blitDebugFrame(TextureHandle handleToWriteOn);

private:
  DebugIndex m_index;
};

} // namespace SirEngine
