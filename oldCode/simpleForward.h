#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {

class SimpleForward final : public GNode {
 public:
  enum PLUGS : uint32_t {
    IN_TEXTURE = inputPlugCode(0),
    DEPTH_RT = inputPlugCode(1),
    OUT_TEXTURE = outputPlugCode(0),
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

}  // namespace SirEngine
