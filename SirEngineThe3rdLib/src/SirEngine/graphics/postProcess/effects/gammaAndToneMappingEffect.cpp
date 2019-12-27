#include "SirEngine/graphics/postProcess/effects/gammaAndToneMappingEffect.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/Dx12PSOManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"

#include "SirEngine/graphics/debugAnnotations.h"

namespace SirEngine {
static const char *GAMMA_TONE_PSO = "gammaAndToneMappingEffect_PSO";
void GammaAndToneMappingEffect::initialize() {
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
      "standardPostProcessEffect_RS");
  pso = dx12::PSO_MANAGER->getHandleFromName(GAMMA_TONE_PSO);

  m_config.exposure = 1.0f;
  m_config.gamma = 2.2f;
  m_config.gammaInverse = 1.0f / m_config.gamma;
  m_constantBufferHandle = globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(GammaToneMappingConfig), &m_config);
}

void GammaAndToneMappingEffect::render(const TextureHandle input,
                                       const TextureHandle output,
                                       const PostProcessResources &) {

  annotateGraphicsBegin("Gamma & Tone-mapping");
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  // check if we need to update the constant buffer
  if (updateConfig) {
    updateConstantBuffer();
  }

  D3D12_RESOURCE_BARRIER barriers[2];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      output, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(output, TextureHandle{});
  dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(input);

  dx12::PSO_MANAGER->bindPSO(pso, commandList);
  commandList->SetGraphicsRootSignature(rs);
  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      2, dx12::CONSTANT_BUFFER_MANAGER
             ->getConstantBufferDx12Handle(m_constantBufferHandle)
             .gpuHandle);

  commandList->DrawInstanced(6, 1, 0, 0);
  annotateGraphicsEnd();
}
void GammaAndToneMappingEffect::updateConstantBuffer() {
  globals::CONSTANT_BUFFER_MANAGER->updateConstantBufferBuffered(
      m_constantBufferHandle, &m_config);
  updateConfig = false;
}

void GammaAndToneMappingEffect::clear() {}
} // namespace SirEngine
