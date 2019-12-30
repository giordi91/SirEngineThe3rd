#include "SirEngine/graphics/nodes/vkSimpleForward.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/layers/vkTempLayer.h"
#include "SirEngine/textureManager.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "platform/windows/graphics/vk/vkTextureManager.h"
#include "platform/windows/graphics/vk/volk.h"

namespace SirEngine {

//void createRenderTargetAndFrameBuffer(VkFramebuffer &outFrameBuffer,
//                                      vk::VkTexture2D &rt, VkRenderPass pass,
//                                      const int width, const int height) {
//  vk::createRenderTarget(
//      "RT", VK_FORMAT_R8G8B8A8_UNORM, vk::LOGICAL_DEVICE, rt,
//      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
//      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, width, height);
//}

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

  const PSOHandle handle =
      vk::PSO_MANAGER->loadRawPSO("../data/pso/forwardPhongPSO.json");
  m_pass = vk::PSO_MANAGER->getRenderPassFromHandle(handle);
}

void VkSimpleForward::initialize() {

  int width = globals::ENGINE_CONFIG->m_windowWidth;
  int height = globals::ENGINE_CONFIG->m_windowHeight;

  m_rtHandle = globals::TEXTURE_MANAGER->allocateRenderTexture(
      width, height, RenderTargetFormat::RGBA32, "simpleForwardRT");

  m_rt = vk::TEXTURE_MANAGER->getTextureData(m_rtHandle);

  VkFramebufferCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  createInfo.renderPass = m_pass;
  createInfo.pAttachments = &m_rt.view;
  createInfo.attachmentCount = 1;
  createInfo.width = m_rt.width;
  createInfo.height = m_rt.height;
  createInfo.layers = 1;

  VK_CHECK(vkCreateFramebuffer(vk::LOGICAL_DEVICE, &createInfo, nullptr,
                               &m_tempFrameBuffer));
}

void VkSimpleForward::compute() {

  // annotateGraphicsBegin("Simple Forward");

  // get input color texture
  /*
  const auto renderTarget =
      getInputConnection<TextureHandle>(m_inConnections, IN_TEXTURE);

  const auto depth =
      getInputConnection<TextureHandle>(m_inConnections, DEPTH_RT);
      */
  static VkClearColorValue color{0.4, 0.4, 0.4, 1};
  // lets us start a render pass

  VkClearValue clear{};
  clear.color = color;

  VkRenderPassBeginInfo beginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  beginInfo.renderPass = m_pass;
  beginInfo.framebuffer = m_tempFrameBuffer;

  // similar to a viewport mostly used on "tiled renderers" to optimize, talking
  // about hardware based tile renderer, aka mobile GPUs.
  beginInfo.renderArea.extent.width =
      static_cast<int32_t>(globals::ENGINE_CONFIG->m_windowWidth);
  beginInfo.renderArea.extent.height =
      static_cast<int32_t>(globals::ENGINE_CONFIG->m_windowHeight);
  beginInfo.clearValueCount = 1;
  beginInfo.pClearValues = &clear;

  vkCmdBeginRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, &beginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  DrawCallConfig config{
      globals::ENGINE_CONFIG->m_windowWidth,
      globals::ENGINE_CONFIG->m_windowHeight,
      static_cast<uint32_t>(DRAW_CALL_FLAGS::SHOULD_CLEAR_COLOR),
      glm::vec4(0.4f, 0.4f, 0.4f, 1.0f),
  };
  globals::RENDERING_CONTEXT->renderQueueType(config,
                                              SHADER_QUEUE_FLAGS::FORWARD);

  vkCmdEndRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
}

void VkSimpleForward::onResizeEvent(int, int) {
  clear();
  initialize();
}

void VkSimpleForward::populateNodePorts() { assert(0); }

void VkSimpleForward::clear() {
  //vk::destroyFrameBuffer(vk::LOGICAL_DEVICE, m_tempFrameBuffer, m_rt);
}
} // namespace SirEngine
