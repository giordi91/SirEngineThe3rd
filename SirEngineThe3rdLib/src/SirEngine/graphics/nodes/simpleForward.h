#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include <d3d12.h>

namespace SirEngine {

class SimpleForward final : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    DEPTH_RT = INPUT_PLUG_CODE(1),
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
    COUNT = 3
  };

public:
  explicit SimpleForward(GraphAllocators &allocators);
  virtual ~SimpleForward() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;

  void populateNodePorts() override;

private:
  // handles
  TextureHandle renderTarget{};
  TextureHandle depth{};
};

} // namespace SirEngine
