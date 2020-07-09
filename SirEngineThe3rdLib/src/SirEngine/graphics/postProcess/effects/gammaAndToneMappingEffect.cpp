#include "SirEngine/graphics/postProcess/effects/gammaAndToneMappingEffect.h"

#include "SirEngine/constantBufferManager.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"

namespace SirEngine {
static const char *GAMMA_TONE_RS = "standardPostProcessEffect_RS";
static const char *GAMMA_TONE_PSO = "gammaAndToneMappingEffect_PSO";
void GammaAndToneMappingEffect::initialize() {
  m_rs = globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(GAMMA_TONE_RS);
  m_pso = globals::PSO_MANAGER->getHandleFromName(GAMMA_TONE_PSO);

  m_config.exposure = 1.0f;
  m_config.gamma = 2.2f;
  m_config.gammaInverse = 1.0f / m_config.gamma;
  m_constantBufferHandle = globals::CONSTANT_BUFFER_MANAGER->allocate(
      sizeof(GammaToneMappingConfig),0, &m_config);

  const char *queues[5] = {nullptr, nullptr, nullptr, nullptr,
                           "gammaAndToneMapping"};
  m_matHandle = globals::MATERIAL_MANAGER->allocateMaterial(
      "gammaAndToneMapping",
      //MaterialManager::ALLOCATE_MATERIAL_FLAG_BITS::BUFFERED, queues);
      0, queues);
}

void GammaAndToneMappingEffect::render(const TextureHandle input,
                                       const TextureHandle output,
                                       const PostProcessResources &) {
  if (updateConfig) {
    updateConstantBuffer();
  }
  globals::MATERIAL_MANAGER->bindTexture(m_matHandle, input, 0, 1,
                                         SHADER_QUEUE_FLAGS::CUSTOM, false);
  globals::MATERIAL_MANAGER->bindConstantBuffer(
      m_matHandle, m_constantBufferHandle, 1, 2, SHADER_QUEUE_FLAGS::CUSTOM);

  globals::MATERIAL_MANAGER->bindMaterial(m_matHandle,
                                          SHADER_QUEUE_FLAGS::CUSTOM);
  globals::RENDERING_CONTEXT->fullScreenPass();
}
void GammaAndToneMappingEffect::updateConstantBuffer() {
  globals::CONSTANT_BUFFER_MANAGER->update(
  //globals::CONSTANT_BUFFER_MANAGER->updateConstantBufferBuffered(
      m_constantBufferHandle, &m_config);
  updateConfig = false;
}

void GammaAndToneMappingEffect::clear() {}
}  // namespace SirEngine
