#include "SirEngine/graphics/nodes/forwardPlus.h"

#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/textureManager.h"

namespace SirEngine {

ForwardPlus::ForwardPlus(GraphAllocators &allocators)
    : GNode("ForwardPlus", "ForwardPlus", allocators) {
  defaultInitializePlugsAndConnections(0, 2);
  GPlug &outTexture = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEXTURE)];
  outTexture.plugValue = 0;
  outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";

  GPlug &depthBuffer = m_outputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
  depthBuffer.plugValue = 0;
  depthBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
  depthBuffer.nodePtr = this;
  depthBuffer.name = "depth";
}

void ForwardPlus::initialize() {
  int width = globals::ENGINE_CONFIG->m_windowWidth;
  int height = globals::ENGINE_CONFIG->m_windowHeight;

  m_rtHandle = globals::TEXTURE_MANAGER->allocateTexture(
      width, height, RenderTargetFormat::R16G16B16A16_FLOAT, "simpleForwardRT",
      TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::RENDER_TARGET |
          TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::SHADER_RESOURCE,
      RESOURCE_STATE::SHADER_READ_RESOURCE);
  m_depthHandle = globals::TEXTURE_MANAGER->allocateTexture(
      width, height, RenderTargetFormat::DEPTH_F32_S8, "simpleForwardDepth",
      TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::DEPTH_TEXTURE |
          TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::SHADER_RESOURCE,
      RESOURCE_STATE::DEPTH_RENDER_TARGET);
}

void ForwardPlus::compute() {
  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandle);

  DrawCallConfig config{globals::ENGINE_CONFIG->m_windowWidth,
                        globals::ENGINE_CONFIG->m_windowHeight, 0};
  globals::RENDERING_CONTEXT->renderQueueType(config,
                                              SHADER_QUEUE_FLAGS::FORWARD);

  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandle);
}

void ForwardPlus::onResizeEvent(int, int) {
  clear();
  initialize();
}

void ForwardPlus::populateNodePorts() {
  // setting the render target output handle
  m_outputPlugs[0].plugValue = m_rtHandle.handle;
  m_outputPlugs[1].plugValue = m_depthHandle.handle;

  // we have everything necessary to prepare the buffers
  FrameBufferBindings bindings{};
  bindings.colorRT[0].handle = m_rtHandle;
  bindings.colorRT[0].clearColor = {0.4, 0.4, 0.4, 1};
  bindings.colorRT[0].shouldClearColor = true;
  bindings.colorRT[0].currentResourceState =
      RESOURCE_STATE::SHADER_READ_RESOURCE;
  bindings.colorRT[0].neededResourceState = RESOURCE_STATE::RENDER_TARGET;
  bindings.colorRT[0].isSwapChainBackBuffer = 0;

  bindings.depthStencil.handle = m_depthHandle;
  // bindings.depthStencil.clearDepthColor= {1.0, 1.0, 1.0, 1.0};
  bindings.depthStencil.clearDepthColor = {0.0, 0.0, 0.0, 0.0};
  bindings.depthStencil.clearStencilColor = {0.0, 0.0, 0.0, 0.0};
  bindings.depthStencil.shouldClearDepth = true;
  bindings.depthStencil.shouldClearStencil = true;
  bindings.depthStencil.currentResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;
  bindings.depthStencil.neededResourceState =
      RESOURCE_STATE::DEPTH_RENDER_TARGET;

  bindings.width = globals::ENGINE_CONFIG->m_windowWidth;
  bindings.height = globals::ENGINE_CONFIG->m_windowHeight;

  m_bindHandle = globals::RENDERING_CONTEXT->prepareBindingObject(
      bindings, "ForwardPlus");
}

void ForwardPlus::clear() {
  if (m_rtHandle.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(m_rtHandle);
  }
  if (m_depthHandle.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(m_depthHandle);
  }
  if (m_bindHandle.isHandleValid()) {
    globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandle);
  }
}
} // namespace SirEngine
