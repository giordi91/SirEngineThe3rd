#include "SirEngine/graphics/nodes/simpleForward.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"

namespace SirEngine {

static const char *SIMPLE_FORWARD_RS = "simpleMeshRSTex";
static const char *SIMPLE_FORWARD_PSO = "simpleMeshPSOTex";

SimpleForward::SimpleForward(GraphAllocators &allocators)
    : GNode("SimpleForward", "SimpleForward", allocators) {

  defaultInitializePlugsAndConnections(3, 1);
  // lets create the plugs
  GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
  inTexture.plugValue = 0;
  inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";

  GPlug &depthBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depthTexture";

  GPlug &outTexture = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEXTURE)];
  outTexture.plugValue = 0;
  outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";

  // fetching root signature
  rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromName(SIMPLE_FORWARD_RS);
  pso = dx12::PSO_MANAGER->getHandleFromName(SIMPLE_FORWARD_PSO);
}

void SimpleForward::initialize() {
  m_brdfHandle = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/brdf.texture");
}

void SimpleForward::compute() {

  annotateGraphicsBegin("Simple Forward");

  // get input color texture
  const auto renderTarget =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);

  const auto depth =
      getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;

  D3D12_RESOURCE_BARRIER barriers[1];
  int counter = 0;
  counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, counter);

  if (counter) {
    commandList->ResourceBarrier(counter, barriers);
  }

  globals::TEXTURE_MANAGER->bindRenderTarget(renderTarget, depth);
  // globals::TEXTURE_MANAGER->bindRenderTarget(renderTarget, TextureHandle{});

  globals::RENDERING_CONTEXT->renderQueueType(SHADER_QUEUE_FLAGS::FORWARD);

  m_outputPlugs[0].plugValue = renderTarget.handle;
  annotateGraphicsEnd();
}


void SimpleForward::onResizeEvent(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
