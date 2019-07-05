#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include <d3d12.h>
#include "SirEngine/graphics/cpuGraphicsStructures.h"

namespace SirEngine {

class ProceduralSkyBoxPass : public GraphNode {
public:
  ProceduralSkyBoxPass(const char *name);
  virtual ~ProceduralSkyBoxPass() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void clear() override;
  virtual void resize(int screenWidth, int screenHeight) override;

private:
  TextureHandle m_skyboxBuffer{};
  ID3D12RootSignature* rs =nullptr;
  PSOHandle pso;
};

} // namespace SirEngine
