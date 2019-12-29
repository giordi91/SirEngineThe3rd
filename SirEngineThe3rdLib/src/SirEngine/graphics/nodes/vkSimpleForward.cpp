#include "SirEngine/graphics/nodes/vkSimpleForward.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "platform/windows/graphics/vk/volk.h"

namespace SirEngine {

static const char *SIMPLE_FORWARD_RS = "simpleMeshRSTex";
static const char *SIMPLE_FORWARD_PSO = "simpleMeshPSOTex";

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

void VkSimpleForward::initialize() {}

void VkSimpleForward::compute() {

  // annotateGraphicsBegin("Simple Forward");

  // get input color texture
  /*
  const auto renderTarget =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);

  const auto depth =
      getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);
      */

  // draw calls go here
  VkViewport viewport{0,
                      float(globals::ENGINE_CONFIG->m_windowHeight),
                      float(globals::ENGINE_CONFIG->m_windowWidth),
                      -float(globals::ENGINE_CONFIG->m_windowHeight),
                      0.0f,
                      1.0f};
  VkRect2D scissor{
      {0, 0},
      {static_cast<uint32_t>(globals::ENGINE_CONFIG->m_windowWidth),
       static_cast<uint32_t>(globals::ENGINE_CONFIG->m_windowHeight)}};
  vkCmdSetViewport(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, 0, 1, &scissor);

  globals::RENDERING_CONTEXT->renderQueueType(SHADER_QUEUE_FLAGS::FORWARD);


}

void VkSimpleForward::onResizeEvent(int, int) {
  clear();
  initialize();
}
} // namespace SirEngine
