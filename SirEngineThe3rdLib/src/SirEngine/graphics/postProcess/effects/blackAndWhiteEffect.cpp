#include "SirEngine/graphics/postProcess/effects/blackAndWhiteEffect.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"

namespace SirEngine {
void BlackAndWhiteEffect::initialize() {
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
      "blackAndWhiteEffect_RS");
  pso = dx12::PSO_MANAGER->getComputePSOByName("blackAndWhiteEffect_PSO");
}

void BlackAndWhiteEffect::render(TextureHandle input, TextureHandle output) {

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_RESOURCE_BARRIER barriers[2];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      output, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  commandList->ResourceBarrier(counter, barriers);

  globals::TEXTURE_MANAGER->bindRenderTarget(output, TextureHandle{});
  dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(input);

  commandList->SetPipelineState(pso);
  commandList->SetGraphicsRootSignature(rs);
  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);
  commandList->DrawInstanced(6, 1, 0, 0);
}

void BlackAndWhiteEffect::clear() {}
} // namespace SirEngine
