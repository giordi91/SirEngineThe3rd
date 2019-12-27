#include "SSSSSEffect.h"
#include "SirEngine/globals.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12MaterialManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/graphics/debugAnnotations.h"

namespace SirEngine {
static const char *SSSSS_RS = "SSSSSEffect_RS";
static const char *SSSSS_PSO = "SSSSSEffect_PSO";
static const char *COPY_RS = "debugFullScreenBlit_RS";
static const char *COPY_PSO = "fragmentCopyPSO";

SSSSSEffect::SSSSSEffect(const char *name) : PostProcessEffect(name, "SSSSS") {
  m_config.correction = 800.0f;
  m_config.maxdd = 0.001f;
  m_config.sssLevel = 31.5f;
  m_config.width = 0.25f;

  m_configW.direction.x = 1.0f;
  m_configW.direction.y = 0.0f;

  m_configH.direction.x = 0.0f;
  m_configH.direction.y = 1.0f;

  copyConfigs();

  m_constantBufferHandleW = globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(SSSSSConfig), &m_configW);
  m_constantBufferHandleH = globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(SSSSSConfig), &m_configH);
}

void SSSSSEffect::initialize() {
  rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(SSSSS_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(SSSSS_PSO);

  copyRs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(COPY_RS);
  copyPso = dx12::PSO_MANAGER->getHandleFromName(COPY_PSO);
}

void SSSSSEffect::render(const TextureHandle input, const TextureHandle output,
                         const PostProcessResources &resources) {
  annotateGraphicsBegin("SSSSS");
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  if (m_updateConfig) {
    globals::CONSTANT_BUFFER_MANAGER->updateConstantBufferBuffered(
        m_constantBufferHandleW, &m_configW);
    globals::CONSTANT_BUFFER_MANAGER->updateConstantBufferBuffered(
        m_constantBufferHandleH, &m_configH);
    m_updateConfig = false;
  }

  const dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(input);
  const dx12::DescriptorPair pairOut = dx12::TEXTURE_MANAGER->getSRVDx12(output);

  D3D12_RESOURCE_BARRIER barriers[3];
  int counter = 0;
  // need to perform a copy of the texture
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      output, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      resources.depth,
      D3D12_RESOURCE_STATE_DEPTH_READ |
          D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
      barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  dx12::TEXTURE_MANAGER->bindRenderTarget(output, TextureHandle{});
  dx12::PSO_MANAGER->bindPSO(copyPso, commandList);
  commandList->SetGraphicsRootSignature(copyRs);
  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);
  commandList->DrawInstanced(6, 1, 0, 0);

  dx12::TEXTURE_MANAGER->bindRenderTargetStencil(output, resources.depth);
  // globals::TEXTURE_MANAGER->bindRenderTarget(output, TextureHandle{});
  const dx12::DescriptorPair depthPair =
      dx12::TEXTURE_MANAGER->getSRVDx12(resources.depth);


  //horizontal pass
  dx12::PSO_MANAGER->bindPSO(pso, commandList);
  commandList->SetGraphicsRootSignature(rs);
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(2, depthPair.gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      3, dx12::CONSTANT_BUFFER_MANAGER
             ->getConstantBufferDx12Handle(m_constantBufferHandleW)
             .gpuHandle);
  commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::SSSSS));
  commandList->DrawInstanced(6, 1, 0, 0);

  //vertical pass 

  counter = 0;
  // need to perform a copy of the texture
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      output, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      input, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  dx12::TEXTURE_MANAGER->bindRenderTargetStencil(input, resources.depth);
  //dx12::TEXTURE_MANAGER->bindRenderTarget(input, resources.depth);
  dx12::PSO_MANAGER->bindPSO(pso, commandList);
  commandList->SetGraphicsRootSignature(rs);
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
  commandList->SetGraphicsRootDescriptorTable(1, pairOut.gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(2, depthPair.gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(
      3, dx12::CONSTANT_BUFFER_MANAGER
             ->getConstantBufferDx12Handle(m_constantBufferHandleH)
             .gpuHandle);

  commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::SSSSS));
  commandList->DrawInstanced(6, 1, 0, 0);

  //need to do another copy
  counter = 0;
  // need to perform a copy of the texture
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      output, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  dx12::TEXTURE_MANAGER->bindRenderTarget(output, TextureHandle{});
  dx12::PSO_MANAGER->bindPSO(copyPso, commandList);
  commandList->SetGraphicsRootSignature(copyRs);
  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);
  commandList->DrawInstanced(6, 1, 0, 0);

  annotateGraphicsEnd();
}

void SSSSSEffect::clear() {}
}  // namespace SirEngine
