#include "SirEngine/graphics/postProcess/effects/gammaAndToneMappingEffect.h"

#include "SirEngine/constantBufferManager.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"

namespace SirEngine {
static const char *GAMMA_TONE_RS = "gammaAndToneMappingEffect_PSO";
static const char *GAMMA_TONE_PSO = "gammaAndToneMappingEffect_PSO";
void GammaAndToneMappingEffect::initialize() {
  m_rs = globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(GAMMA_TONE_RS);
  m_pso = globals::PSO_MANAGER->getHandleFromName(GAMMA_TONE_PSO);

  m_config.exposure = 1.0f;
  m_config.gamma = 2.2f;
  m_config.gammaInverse = 1.0f / m_config.gamma;
  m_constantBufferHandle = globals::CONSTANT_BUFFER_MANAGER->allocate(
      sizeof(GammaToneMappingConfig), 0, &m_config);

  graphics::BindingDescription descriptions[2] = {
      {0, GRAPHIC_RESOURCE_TYPE::TEXTURE,
       GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT},
      {1, GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT}};
  m_bindingTable = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
      descriptions, 2,
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
      "gammaAndToneMappingBindingTable");
}

void GammaAndToneMappingEffect::render(const TextureHandle input,
                                       const TextureHandle,
                                       const PostProcessResources &) {
  if (updateConfig) {
    updateConstantBuffer();
  }
  globals::PSO_MANAGER->bindPSO(m_pso);
  globals::BINDING_TABLE_MANAGER->bindTexture(m_bindingTable, input, 0, 0,
                                              false);
  globals::BINDING_TABLE_MANAGER->bindConstantBuffer(
      m_bindingTable, m_constantBufferHandle, 1, 1);
  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_OBJECT_BINDING_INDEX, m_bindingTable, m_rs);
  globals::RENDERING_CONTEXT->fullScreenPass();
}
void GammaAndToneMappingEffect::updateConstantBuffer() {
  globals::CONSTANT_BUFFER_MANAGER->update(m_constantBufferHandle, &m_config);
  updateConfig = false;
}

void GammaAndToneMappingEffect::clear() {
  if (m_bindingTable.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->free(m_bindingTable);
    m_bindingTable = {};
  }
  if (m_constantBufferHandle.isHandleValid()) {
    globals::CONSTANT_BUFFER_MANAGER->free(m_constantBufferHandle);
    m_constantBufferHandle = {};
  }
}
}  // namespace SirEngine
