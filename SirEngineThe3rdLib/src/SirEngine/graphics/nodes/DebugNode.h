#pragma once

#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {
class DebugNode final : public GraphNode {
public:
  DebugNode(const char *name);
  virtual ~DebugNode() = default;
  virtual void initialize() override;
  virtual void compute() override;
  void setDebugIndex(int index) { m_index = static_cast<DebugIndex>(index); }
  void setConfig(DebugLayerConfig config) {
    m_config = config;
    updateConfig = true;
  }

private:
  enum DebugIndex {
    GBUFFER = 1,
    NORMAL_BUFFER,
    SPECULAR_BUFFER,
    GBUFFER_DEPTH,
  };

private:
  void blitDebugFrame(TextureHandle handleToWriteOn);
  void updateConstantBuffer();

private:
  DebugIndex m_index;
  DebugLayerConfig m_config;
  ConstantBufferHandle m_constBufferHandle;
  bool updateConfig = false;
};

} // namespace SirEngine
