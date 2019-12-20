#include "SirEngine/graphics/nodes/shadowPass.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/dx12SwapChain.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/textureManagerDx12.h"

namespace SirEngine {
ShadowPass::ShadowPass(GraphAllocators &allocators)
    : GNode("ShadowPass", "ShadowPass", allocators) {

  defaultInitializePlugsAndConnections(1, 4);
  // lets create the plugs
  GPlug &geometryBuffer =
      m_outputPlugs[PLUG_INDEX(PLUGS::DIRECTIONAL_SHADOW_RT)];
  geometryBuffer.plugValue = 0;
  geometryBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  geometryBuffer.nodePtr = this;
  geometryBuffer.name = "shadowRT";

}

void ShadowPass::initialize() {
  m_shadow = dx12::TEXTURE_MANAGER->createDepthTexture("directionalShadow",
                                                       shadowSize, shadowSize);
}

void ShadowPass::compute() {

  annotateGraphicsBegin("ShadowPass");

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  // first thing first we need to be able to bind the deferred buffers,
  // to do so we first transition them to be render targets
  D3D12_RESOURCE_BARRIER barriers[4];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      m_shadow, D3D12_RESOURCE_STATE_DEPTH_WRITE, barriers, counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  // now that they are in the right state we are going to clear them
  globals::TEXTURE_MANAGER->clearDepth(m_shadow, 0.0f);

  auto depthDescriptor = dx12::TEXTURE_MANAGER->getRTVDx12(m_shadow).cpuHandle;
  commandList->OMSetRenderTargets(0, nullptr, false, &depthDescriptor);

  // Set the viewport and scissor rect.  This needs to be reset whenever the
  // command list is reset.
  auto viewport =
      D3D12_VIEWPORT{0.0f, 0.0f, shadowSize, shadowSize, 0.0f, 1.0f};

  D3D12_RECT scissorRect{0, 0, shadowSize, shadowSize};
  commandList->RSSetViewports(1, &viewport);
  commandList->RSSetScissorRects(1, &scissorRect);

  globals::RENDERING_CONTEXT->renderQueueType(SHADER_QUEUE_FLAGS::SHADOW);

  // TODO remove this each draw call should set its own
  // Set the viewport and scissor rect.  This needs to be reset whenever the
  // command list is reset.
  commandList->RSSetViewports(1, dx12::SWAP_CHAIN->getViewport());
  commandList->RSSetScissorRects(1, dx12::SWAP_CHAIN->getScissorRect());

  // setting the data as output
  m_outputPlugs[0].plugValue = m_shadow.handle;

  // if we are in debug we want to populate debug data such that can be
  // used for blitting debug data on screen
  globals::DEBUG_FRAME_DATA->directionalShadow = m_shadow;
  annotateGraphicsEnd();
}

inline void freeTextureIfValid(TextureHandle h) {
  if (h.isHandleValid()) {
    dx12::TEXTURE_MANAGER->free(h);
    h.handle = 0;
  }
}

void ShadowPass::clear() {
  freeTextureIfValid(m_shadow);
  m_generation = -1;
}

void ShadowPass::onResizeEvent(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
