#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include <d3d12.h>
#include "platform/windows/graphics/dx12/meshManager.h"

namespace SirEngine {

class SkyBoxPass : public GraphNode {
public:
  SkyBoxPass(const char *name);
  virtual ~SkyBoxPass() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void clear() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

private:
  ID3D12RootSignature* rs =nullptr;
  PSOHandle pso;
  dx12::MeshRuntime m_meshRuntime;
};

} // namespace SirEngine
