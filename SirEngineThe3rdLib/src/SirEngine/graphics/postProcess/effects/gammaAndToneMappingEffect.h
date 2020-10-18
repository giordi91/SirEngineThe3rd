#pragma once
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include "SirEngine/handle.h"

namespace SirEngine {
class GammaAndToneMappingEffect final : public PostProcessEffect {
 public:
  explicit GammaAndToneMappingEffect(const char *name)
      : PostProcessEffect(name, "GammaAndToneMappingEffect") {}
  virtual ~GammaAndToneMappingEffect() = default;
  virtual void initialize() override;
  virtual void render(const TextureHandle input, const TextureHandle output,
                      const PostProcessResources &resources) override;
  virtual void clear() override;
  GammaToneMappingConfig &getConfig() { return m_config; };
  void setConfigDirty() {
    m_config.gammaInverse = 1.0f / m_config.gamma;
    updateConfig = true;
  }

 private:
  void updateConstantBuffer();

 private:
  RSHandle m_rs{};
  PSOHandle m_pso{};
  ConstantBufferHandle m_constantBufferHandle{};
  bool updateConfig = false;
  GammaToneMappingConfig m_config;
  BindingTableHandle m_bindingTable{};
};

}  // namespace SirEngine
