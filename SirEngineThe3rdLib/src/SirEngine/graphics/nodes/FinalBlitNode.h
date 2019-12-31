#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include <d3d12.h>
#include "SirEngine/handle.h"

namespace SirEngine {
class FinalBlitNode final : public GNode {
public:
  enum PLUGS { IN_TEXTURE = INPUT_PLUG_CODE(0), COUNT = 1 };
public:
  explicit FinalBlitNode(GraphAllocators &allocators);
  virtual ~FinalBlitNode() = default;
  virtual void compute() override;
  virtual void initialize() override;
  void populateNodePorts() override;
  void clear() override;
private:
  ID3D12RootSignature *m_rs = nullptr;
  PSOHandle m_pso{};
  TextureHandle inputRTHandle;
  MaterialHandle m_matHandle{};
  BufferBindingsHandle m_bindHandle{};
};

} // namespace SirEngine
