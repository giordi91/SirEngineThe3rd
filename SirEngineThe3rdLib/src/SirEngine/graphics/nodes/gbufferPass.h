#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include <d3d12.h>

namespace SirEngine {

class GBufferPass : public GraphNode {
public:
  GBufferPass(const char *name);
  virtual ~GBufferPass() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void clear() override;
  virtual void resize(int screenWidth, int screenHeight) override;

private:
  TextureHandle m_geometryBuffer{};
  TextureHandle m_normalBuffer{};
  TextureHandle m_specularBuffer{};
  TextureHandle m_depth{};
  ID3D12RootSignature* rs =nullptr;
  ID3D12PipelineState* pso = nullptr;
};

} // namespace SirEngine
