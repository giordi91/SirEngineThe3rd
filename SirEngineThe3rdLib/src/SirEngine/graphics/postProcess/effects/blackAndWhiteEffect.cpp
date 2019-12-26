#include "SirEngine/graphics/postProcess/effects/blackAndWhiteEffect.h"
#include "SirEngine/globals.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"

namespace SirEngine {
static const char *BW_RS = "blackAndWhiteEffect_RS";
static const char *BW_PSO = "blackAndWhiteEffect_PSO";

BlackAndWhiteEffect::BlackAndWhiteEffect(const char *name)
    : PostProcessEffect(name, "BlackAndWhiteEffect") {}

void BlackAndWhiteEffect::initialize() {
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(BW_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(BW_PSO);
}

void BlackAndWhiteEffect::render(const TextureHandle input,
                                 const TextureHandle output,
                                 const PostProcessResources &) {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

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
  const dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(input);

  dx12::PSO_MANAGER->bindPSO(pso, commandList);
  commandList->SetGraphicsRootSignature(rs);
  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);
  commandList->DrawInstanced(6, 1, 0, 0);
}

void BlackAndWhiteEffect::clear() {}
}  // namespace SirEngine
