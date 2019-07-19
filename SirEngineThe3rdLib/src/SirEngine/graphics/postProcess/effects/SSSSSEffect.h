#pragma once
#include <d3d12.h>
#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include "SirEngine/handle.h"

namespace SirEngine {
class SSSSSEffect final : public PostProcessEffect {
 public:
  explicit SSSSSEffect(const char *name);
  virtual ~SSSSSEffect()=default;
  void initialize() override;
  void render(const TextureHandle input, const TextureHandle output,
              const PostProcessResources &resources) override;
  void clear() override;

 private:
  ID3D12RootSignature *rs = nullptr;
  PSOHandle pso = {};
};

}  // namespace SirEngine
