#include "SSSSSEffect.h"
#include "SirEngine/globals.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"

namespace SirEngine {
static const char *SSSSS_RS = "SSSSSEffect_RS";
static const char *SSSSS_PSO = "SSSSSEffect_PSO";

SSSSSEffect::SSSSSEffect(const char *name)
    : PostProcessEffect(name, "SSSSSEffect") {}

void SSSSSEffect::initialize() {
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(SSSSS_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(SSSSS_PSO);
}

void SSSSSEffect::render(const TextureHandle input, const TextureHandle output,
                         const PostProcessResources &resources) {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_RESOURCE_BARRIER barriers[3];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      output, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      resources.depth, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers,
      counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(output, resources.depth);
  const dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(input);

  dx12::PSO_MANAGER->bindPSO(pso, commandList);
  commandList->SetGraphicsRootSignature(rs);
  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);
  commandList->DrawInstanced(6, 1, 0, 0);
}

void SSSSSEffect::clear() {}
}  // namespace SirEngine
