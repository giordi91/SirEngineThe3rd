#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include <d3d12.h>

namespace SirEngine {
class FinalBlitNode final : public GraphNode {
public:
  FinalBlitNode();
  virtual ~FinalBlitNode() = default;
  virtual void compute()override;
  virtual void initialize() override;
private:
	ID3D12RootSignature* m_rs = nullptr;
	ID3D12PipelineState* m_pso = nullptr;
};

} // namespace SirEngine
