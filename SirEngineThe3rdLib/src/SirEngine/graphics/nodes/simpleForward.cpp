#include "SirEngine/graphics/nodes/simpleForward.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/Dx12PSOManager.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"

namespace SirEngine {


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

}

void SimpleForward::initialize() {}

void SimpleForward::compute() {

  annotateGraphicsBegin("Simple Forward");

  // get input color texture

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

  globals::RENDERING_CONTEXT->renderQueueType({}, SHADER_QUEUE_FLAGS::FORWARD);

  annotateGraphicsEnd();
}

void SimpleForward::onResizeEvent(int, int) {
  clear();
  initialize();
}

void SimpleForward::populateNodePorts() {
  renderTarget = getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);
  depth = getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);
  m_outputPlugs[0].plugValue = renderTarget.handle;
}
} // namespace SirEngine
