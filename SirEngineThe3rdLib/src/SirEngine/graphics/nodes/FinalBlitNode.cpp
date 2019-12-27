
#include "SirEngine/graphics/nodes/FinalBlitNode.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/handle.h"
#include "WinPixEventRuntime/WinPixEventRuntime/pix3.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/Dx12PSOManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"
#include "platform/windows/graphics/dx12/dx12rootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12SwapChain.h"

namespace SirEngine {

static const char *FINAL_BLIT_PSO = "HDRtoSDREffect_PSO";
static const char *FINAL_BLIT_RS = "standardPostProcessEffect_RS";
FinalBlitNode::FinalBlitNode(GraphAllocators &allocators)
    : GNode("FinalBlit", "FinalBlit", allocators) {
  // lets create the plugs
  defaultInitializePlugsAndConnections(1, 0);

  GPlug &inTexture= m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";
}

void FinalBlitNode::compute() {
  // get the render texture
  annotateGraphicsBegin("EndOfFrameBlit");

  const auto conn = m_inConnections[PLUG_INDEX(PLUGS::IN_TEXTURE)];
  assert(conn->size() == 1 && "too many input connections");
  const GPlug *source = (*conn)[0];
  TextureHandle texH;
  texH.handle = source->plugValue;

  const TextureHandle destination = dx12::SWAP_CHAIN->currentBackBufferTexture();
  // globals::TEXTURE_MANAGER->copyTexture(texH,destination);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_RESOURCE_BARRIER barriers[2];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      texH, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barriers, counter);
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      destination, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);
  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(destination, TextureHandle{});
  dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(texH);

  dx12::PSO_MANAGER->bindPSO(m_pso, commandList);
  commandList->SetGraphicsRootSignature(m_rs);
  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);

  commandList->DrawInstanced(6, 1, 0, 0);
  annotateGraphicsEnd();
}

void FinalBlitNode::initialize() {
  m_rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(FINAL_BLIT_RS);
  m_pso = dx12::PSO_MANAGER->getHandleFromName(FINAL_BLIT_PSO);
}
} // namespace SirEngine
