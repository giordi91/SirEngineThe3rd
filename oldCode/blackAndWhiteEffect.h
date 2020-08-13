#pragma once
#include <d3d12.h>
#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include "SirEngine/handle.h"

namespace SirEngine {
class BlackAndWhiteEffect final : public PostProcessEffect {
 public:
  explicit BlackAndWhiteEffect(const char *name);
  virtual ~BlackAndWhiteEffect() = default;
  void initialize() override;
  void render(const TextureHandle input, const TextureHandle output,
              const PostProcessResources &resources) override;
  void clear() override;

 private:
  ID3D12RootSignature *rs = nullptr;
  PSOHandle pso = {};
};

}  // namespace SirEngine
