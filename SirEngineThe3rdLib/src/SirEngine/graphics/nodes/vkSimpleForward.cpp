#include "SirEngine/graphics/nodes/vkSimpleForward.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/textureManager.h"

namespace SirEngine {

VkSimpleForward::VkSimpleForward(GraphAllocators &allocators)
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

void VkSimpleForward::initialize() {

  int width = globals::ENGINE_CONFIG->m_windowWidth;
  int height = globals::ENGINE_CONFIG->m_windowHeight;

  m_rtHandle = globals::TEXTURE_MANAGER->allocateTexture(
      width, height, RenderTargetFormat::RGBA32, "simpleForwardRT");
}

void VkSimpleForward::compute() {

  globals::RENDERING_CONTEXT->setBindingObject(m_bindHandle);

  DrawCallConfig config{
      globals::ENGINE_CONFIG->m_windowWidth,
      globals::ENGINE_CONFIG->m_windowHeight,
      static_cast<uint32_t>(DRAW_CALL_FLAGS::SHOULD_CLEAR_COLOR),
      glm::vec4(0.4f, 0.4f, 0.4f, 1.0f),
  };
  globals::RENDERING_CONTEXT->renderQueueType(config,
                                              SHADER_QUEUE_FLAGS::FORWARD);

  globals::RENDERING_CONTEXT->clearBindingObject(m_bindHandle);
}

void VkSimpleForward::onResizeEvent(int, int) {
  clear();
  initialize();
}

void VkSimpleForward::populateNodePorts() {
  // setting the render target output handle
  m_outputPlugs[0].plugValue = m_rtHandle.handle;

  // we have everything necessary to prepare the buffers
  FrameBufferBindings bindings{};
  bindings.colorRT[0].handle = m_rtHandle;
  bindings.colorRT[0].clearColor = {0.4, 0.4, 0.4, 1};
  bindings.colorRT[0].shouldClearColor = true;
  bindings.width = globals::ENGINE_CONFIG->m_windowWidth;
  bindings.height = globals::ENGINE_CONFIG->m_windowHeight;

  m_bindHandle = globals::RENDERING_CONTEXT->prepareBindingObject(
      bindings, "vkSimpleForward");
}

void VkSimpleForward::clear() {
  if (m_rtHandle.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(m_rtHandle);
  }
  if (m_bindHandle.isHandleValid()) {
    globals::RENDERING_CONTEXT->freeBindingObject(m_bindHandle);
  }
}
} // namespace SirEngine
