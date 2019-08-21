#pragma once

#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

struct ID3D12RootSignature;
namespace SirEngine {
class FramePassDebugNode final : public GraphNode {
public:
  explicit FramePassDebugNode(const char *name);
  virtual ~FramePassDebugNode() = default;
  virtual void initialize() override;
  virtual void compute() override;
  void setDebugIndex(int index) { m_index = static_cast<DebugIndex>(index); }
  void setConfig(const DebugLayerConfig config) {
    m_config = config;
    m_updateConfig = true;
  }

private:
  enum class DebugIndex {
    GBUFFER = 1,
    NORMAL_BUFFER,
    METALLIC_BUFFER,
    ROUGHNESS_BUFFER,
    THICKNESS_BUFFER,
    GBUFFER_DEPTH,
    GBUFFER_STENCIL
  };

private:
  void blitDebugFrame(TextureHandle handleToWriteOn) const;
  void updateConstantBuffer();
  void reduceDepth(TextureHandle source) const;

private:
  DebugIndex m_index;
  DebugLayerConfig m_config;
  TextureConfig m_textureConfig{0,0};
  ConstantBufferHandle m_constBufferHandle;
  BufferHandle m_reduceBufferHandle;
  bool m_updateConfig = false;

  PSOHandle m_gbufferPSOHandle;
  PSOHandle m_normalPSOHandle;
  PSOHandle m_roughnessPSOHandle;
  PSOHandle m_metallicPSOHandle;
  PSOHandle m_thicknessPSOHandle;
  PSOHandle m_stencilPSOHandle;
  PSOHandle m_depthPSOHandle;
  PSOHandle m_depthReducePSOHandle;
  PSOHandle m_depthReduceClearPSOHandle;
  ID3D12RootSignature *m_rs = nullptr;
  ID3D12RootSignature *m_reduceRs= nullptr;
};

} // namespace SirEngine
