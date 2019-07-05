#pragma once
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include <d3d12.h>
#include "SirEngine/handle.h"

namespace SirEngine {
class GammaAndToneMappingEffect : public PostProcessEffect {
public:
  GammaAndToneMappingEffect(const char *name)
      : PostProcessEffect(name, "GammaAndToneMappingEffect") {}
  virtual ~GammaAndToneMappingEffect() = default;
  virtual void initialize() override;
  virtual void render(TextureHandle input, TextureHandle output) override;
  virtual void clear() override;
  GammaToneMappingConfig &getConfig() { return m_config; };
  void setConfigDirty() {
    m_config.gammaInverse = 1.0f / m_config.gamma;
    updateConfig = true;
  }

private:
  void updateConstantBuffer();

private:
  ID3D12RootSignature *rs;
  PSOHandle pso;
  ConstantBufferHandle m_constantBufferHandle;
  bool updateConfig = false;
  GammaToneMappingConfig m_config;
};

} // namespace SirEngine
