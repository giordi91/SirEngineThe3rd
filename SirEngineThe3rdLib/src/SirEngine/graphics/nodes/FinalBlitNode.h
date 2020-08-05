#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {
class FinalBlitNode final : public GNode {
public:
  enum PLUGS { IN_TEXTURE = inputPlugCode(0), COUNT = 1 };
public:
  explicit FinalBlitNode(GraphAllocators &allocators);
  virtual ~FinalBlitNode() = default;
  virtual void compute() override;
  virtual void initialize() override;
  void populateNodePorts() override;
  void clear() override;
private:
  TextureHandle inputRTHandle;
  MaterialHandle m_matHandle{};
  BufferBindingsHandle m_bindHandle{};
};

} // namespace SirEngine
