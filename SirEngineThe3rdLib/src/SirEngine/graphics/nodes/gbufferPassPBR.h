#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include <d3d12.h>

namespace SirEngine {

class GBufferPassPBR : public GraphNode {
public:
  GBufferPassPBR(const char *name);
  virtual ~GBufferPassPBR() { clear(); };
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
  PSOHandle pso;
};

} // namespace SirEngine
