#pragma once

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine {

class ShadowPass final : public GNode {
public:
  enum PLUGS {
    DIRECTIONAL_SHADOW_RT = OUTPUT_PLUG_CODE(0),
    COUNT = 1
  };

public:
  explicit ShadowPass(GraphAllocators &allocators);
  virtual ~ShadowPass() { clear(); };
  virtual void initialize() override;
  virtual void compute() override;
  virtual void clear() override;
  virtual void onResizeEvent(int screenWidth, int screenHeight) override;
  static constexpr int shadowSize = 4096;

private:
  TextureHandle m_shadow{};
};

} // namespace SirEngine
