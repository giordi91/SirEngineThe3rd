
#include "SirEngine/graphics/nodes/FinalBlitNode.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/handle.h"
#include "WinPixEventRuntime/WinPixEventRuntime/pix3.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/swapChain.h"

namespace SirEngine {

FinalBlitNode::FinalBlitNode() : GraphNode("FinalBlit", "FinalBlit") {
  // lets create the plugs
  Plug inTexture;
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";
  registerPlug(inTexture);
}

void FinalBlitNode::compute() {
  // get the render texture
  annotateGraphicsBegin("EndOfFrameBlit");

  auto &conn = m_connections[&m_inputPlugs[0]];
  assert(conn.size() == 1 && "too many input connections");
  Plug *source = conn[0];
  TextureHandle texH;
  texH.handle = source->plugValue;

  TextureHandle destination = dx12::SWAP_CHAIN->currentBackBufferTexture();
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

  commandList->SetPipelineState(m_pso);
  commandList->SetGraphicsRootSignature(m_rs);
  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);


  commandList->DrawInstanced(6, 1, 0, 0);
  annotateGraphicsEnd();
}

void FinalBlitNode::initialize() {
  m_rs = dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(
      "standardPostProcessEffect_RS");
  m_pso = dx12::PSO_MANAGER->getComputePSOByName("HDRtoSDREffect_PSO");
}
} // namespace SirEngine
