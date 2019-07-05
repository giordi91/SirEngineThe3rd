#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include <d3d12.h>
#include "SirEngine/handle.h"

namespace SirEngine {
class FinalBlitNode final : public GraphNode {
public:
  FinalBlitNode();
  virtual ~FinalBlitNode() = default;
  virtual void compute() override;
  virtual void initialize() override;

private:
  ID3D12RootSignature *m_rs = nullptr;
  PSOHandle m_pso;
};

} // namespace SirEngine
