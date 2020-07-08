#pragma once

#include "SirEngine/graphics/nodeGraph.h"

namespace SirEngine {

class ForwardPlus final : public GNode {
 public:
  enum PLUGS {
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
    DEPTH_RT = OUTPUT_PLUG_CODE(1),
    COUNT = 2,
  };

 public:
  explicit ForwardPlus(GraphAllocators& allocators);
  virtual ~ForwardPlus()=default;
  virtual void initialize() override;
  virtual void compute() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

  void populateNodePorts() override;
  void clear() override;

 private:
  TextureHandle m_rtHandle{};
  TextureHandle m_depthHandle{};
  BufferBindingsHandle m_bindHandle{};
};

}  // namespace SirEngine
