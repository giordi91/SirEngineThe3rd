#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {

class GrassNode final : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    DEPTH_RT = INPUT_PLUG_CODE(1),
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
    OUT_DEPTH= OUTPUT_PLUG_CODE(1),
    COUNT = 4
  };

public:
  explicit GrassNode(GraphAllocators &allocators);
  virtual ~GrassNode() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

  void populateNodePorts() override;

private:
  // handles
  TextureHandle renderTarget{};
  TextureHandle depth{};
  BufferBindingsHandle m_bindHandle{};
};

} // namespace SirEngine
