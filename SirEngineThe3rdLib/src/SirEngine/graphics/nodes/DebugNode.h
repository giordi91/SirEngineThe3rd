#pragma once

#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

struct ID3D12RootSignature;
namespace SirEngine {
class DebugNode final : public GraphNode {
public:
  explicit DebugNode(const char *name);
  virtual ~DebugNode() = default;
  virtual void initialize() override;
  virtual void compute() override;
  void setDebugIndex(int index) { m_index = static_cast<DebugIndex>(index); }
  void setConfig(const DebugLayerConfig config) {
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
  void reduceDepth(TextureHandle source);

private:
  DebugIndex m_index;
  DebugLayerConfig m_config;
  TextureConfig m_textureConfig{0,0};
  ConstantBufferHandle m_constBufferHandle;
  ConstantBufferHandle m_textureConfigHandle;
  BufferHandle m_reduceBufferHandle;
  bool updateConfig = false;

  PSOHandle gbufferPSOHandle;
  PSOHandle normalPSOHandle;
  PSOHandle specularPSOHandle;
  PSOHandle depthPSOHandle;
  PSOHandle depthReducePSOHandle;
  PSOHandle depthReduceClearPSOHandle;
  ID3D12RootSignature *rs = nullptr;
  ID3D12RootSignature *reduceRs= nullptr;
};

} // namespace SirEngine
