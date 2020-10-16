#include "SirEngine/graphics/nodes/debugDrawNode.h"

#include "SirEngine/graphics/debugRenderer.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/handle.h"

namespace SirEngine {

DebugDrawNode::DebugDrawNode(GraphAllocators &allocators)
    : GNode("DebugDrawNode", "DebugDrawNode", allocators) {
  defaultInitializePlugsAndConnections(2, 1);
  // lets create the plugs
  GPlug &inTexture = m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
  inTexture.plugValue = 0;
  inTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
  inTexture.nodePtr = this;
  inTexture.name = "inTexture";

  GPlug &inDepth = m_inputPlugs[getPlugIndex(PLUGS::DEPTH_RT)];
  inDepth.plugValue = 0;
  inDepth.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
  inDepth.nodePtr = this;
  inDepth.name = "depthTexture";

  GPlug &outTexture = m_outputPlugs[getPlugIndex(PLUGS::OUT_TEXTURE)];
  outTexture.plugValue = 0;
  outTexture.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";
}

void DebugDrawNode::initialize(CommandBufferHandle commandBuffer) {}

void DebugDrawNode::compute() {
  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandle);
  globals::DEBUG_RENDERER->render();
  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandle);

  m_outputPlugs[0].plugValue = inputRTHandle.handle;
}

void DebugDrawNode::populateNodePorts() {
  inputRTHandle =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);
  inputDepthHandle =
      getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);
  m_outputPlugs[0].plugValue = inputRTHandle.handle;

  // we have everything necessary to prepare the buffers
  FrameBufferBindings bindings{};
  bindings.colorRT[0].handle = inputRTHandle;
  bindings.colorRT[0].shouldClearColor = false;
  bindings.colorRT[0].currentResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].neededResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].isSwapChainBackBuffer = 0;

  bindings.depthStencil.handle = inputDepthHandle;
  bindings.depthStencil.shouldClearDepth = false;
  bindings.depthStencil.shouldClearStencil = false;
  bindings.depthStencil.currentResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;
  bindings.depthStencil.neededResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;

  bindings.width = globals::ENGINE_CONFIG->m_windowWidth;
  bindings.height = globals::ENGINE_CONFIG->m_windowHeight;

  m_bindHandle = globals::RENDERING_CONTEXT->prepareBindingObject(
      bindings, "DebugDrawPass");
}

void DebugDrawNode::clear() {
    clearResolutionDepenantResources();
}

void DebugDrawNode::clearResolutionDepenantResources()
{
  if (m_bindHandle.isHandleValid()) {
    globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandle);
  }
}

void DebugDrawNode::onResizeEvent(int, int, const CommandBufferHandle commandBuffer)
{
    clearResolutionDepenantResources();
	initializeResolutionDepenantResources(commandBuffer);
}
}  // namespace SirEngine
