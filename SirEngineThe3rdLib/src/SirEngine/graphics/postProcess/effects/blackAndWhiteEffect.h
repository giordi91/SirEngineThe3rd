#pragma once
#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include "SirEngine/handle.h"
#include <d3d12.h>

namespace SirEngine {
class BlackAndWhiteEffect : public PostProcessEffect {
public:
  BlackAndWhiteEffect(const char *name)
      : PostProcessEffect(name, "BlackAndWhiteEffect") {}
  virtual ~BlackAndWhiteEffect() = default;
  virtual void initialize() override;
  virtual void render(TextureHandle input, TextureHandle output) override;
  virtual void clear() override;

private:
  ID3D12RootSignature *rs;
  PSOHandle pso;
};

} // namespace SirEngine
