#include "SirEngine/application.h"
#include "SirEngine/constantBufferManager.h"
#include "SirEngine/events/debugEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"

#include "SirEngine/assetManager.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/layers/vkTempLayer.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkConstantBufferManager.h"
#include "platform/windows/graphics/vk/vkDescriptorManager.h"
#include "platform/windows/graphics/vk/vkLoad.h"
#include "platform/windows/graphics/vk/vkMaterialManager.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "platform/windows/graphics/vk/vkShaderManager.h"
#include "platform/windows/graphics/vk/vkSwapChain.h"
#include "platform/windows/graphics/vk/volk.h"

namespace SirEngine {
void VkTempLayer::createRenderTargetAndFrameBuffer(const int width,
                                                   const int height) {
  vk::createRenderTarget(
      "RT", VK_FORMAT_R8G8B8A8_UNORM, vk::LOGICAL_DEVICE, m_rt,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, width, height);

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

void VkTempLayer::onAttach() {
  globals::MAIN_CAMERA = new Camera3DPivot();
  // globals::MAIN_CAMERA->setLookAt(0, 125, 0);
  // globals::MAIN_CAMERA->setPosition(00, 125, 60);
  CameraManipulationConfig camConfig{
      -0.01f, 0.01f, 0.012f, 0.012f, -0.07f,
  };
  globals::MAIN_CAMERA->setManipulationMultipliers(camConfig);

  globals::MAIN_CAMERA->setLookAt(0, 15, 0);
  globals::MAIN_CAMERA->setPosition(0, 15, 15);
  globals::MAIN_CAMERA->updateCamera();

  // globals::ASSET_MANAGER->loadScene(globals::ENGINE_CONFIG->m_startScenePath);
  globals::ASSET_MANAGER->loadScene("../data/scenes/tempScene.json");

  const PSOHandle handle =
      vk::PSO_MANAGER->loadRawPSO("../data/pso/forwardPhongPSO.json");
  m_pass = vk::PSO_MANAGER->getRenderPassFromHandle(handle);

  createRenderTargetAndFrameBuffer(globals::ENGINE_CONFIG->m_windowWidth,
                                   globals::ENGINE_CONFIG->m_windowHeight);
  // if constexpr (!USE_PUSH) {
  //}

  alloc =
      new GraphAllocators{globals::STRING_POOL, globals::PERSISTENT_ALLOCATOR};
  m_forward = new VkSimpleForward(*alloc);
  m_forward->initialize();
}

void VkTempLayer::onDetach() {}
void VkTempLayer::onUpdate() {

  globals::RENDERING_CONTEXT->setupCameraForFrame();

  static float step = 0.01f;
  static int index = 0;
  static int counter = 0;

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

  // if constexpr (USE_PUSH) {
  //  VkDescriptorBufferInfo bufferInfo{};
  //  bufferInfo.buffer = m_vertexBuffer.buffer;
  //  bufferInfo.offset = 0;
  //  bufferInfo.range = m_vertexBuffer.size;

  //  VkWriteDescriptorSet descriptor[1]{};
  //  descriptor[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  //  descriptor[0].dstBinding = 0;
  //  descriptor[0].descriptorCount = 1;
  //  descriptor[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  //  descriptor[0].pBufferInfo = &bufferInfo;

  //  vkCmdPushDescriptorSetKHR(FRAME_COMMAND[0].m_commandBuffer,
  // VK_PIPELINE_BIND_POINT_GRAPHICS, PIPELINE_LAYOUT, 0, ARRAYSIZE(descriptor),
  //                            descriptor);

  m_forward->compute();

  vkCmdEndRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer);

  VkImageMemoryBarrier barrier[2] = {};
  barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  barrier[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier[0].srcQueueFamilyIndex = 0;
  barrier[0].dstQueueFamilyIndex = 0;
  barrier[0].image = m_rt.image;
  barrier[0].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier[0].subresourceRange.baseArrayLayer = 0;
  barrier[0].subresourceRange.baseMipLevel = 0;
  barrier[0].subresourceRange.levelCount = 1;
  barrier[0].subresourceRange.layerCount = 1;
  barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier[1].srcQueueFamilyIndex = 0;
  barrier[1].dstQueueFamilyIndex = 0;
  barrier[1].image = vk::SWAP_CHAIN->images[globals::CURRENT_FRAME];
  barrier[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier[1].subresourceRange.baseArrayLayer = 0;
  barrier[1].subresourceRange.baseMipLevel = 0;
  barrier[1].subresourceRange.levelCount = 1;
  barrier[1].subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(
      vk::CURRENT_FRAME_COMMAND->m_commandBuffer,
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
      VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 0, nullptr, 2, barrier);

  VkImageCopy region{};
  region.dstSubresource.layerCount = 1;
  region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.dstSubresource.baseArrayLayer = 0;
  region.dstSubresource.mipLevel = 0;
  region.srcSubresource.layerCount = 1;
  region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.srcSubresource.baseArrayLayer = 0;
  region.srcSubresource.mipLevel = 0;
  region.dstOffset = VkOffset3D{};
  region.srcOffset = VkOffset3D{};
  region.extent.width = m_rt.width;
  region.extent.height = m_rt.height;
  region.extent.depth = 1;

  vkCmdCopyImage(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, m_rt.image,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 vk::SWAP_CHAIN->images[globals::CURRENT_FRAME],
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  barrier[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  barrier[0].srcQueueFamilyIndex = 0;
  barrier[0].dstQueueFamilyIndex = 0;
  barrier[0].image = m_rt.image;
  barrier[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier[0].subresourceRange.baseArrayLayer = 0;
  barrier[0].subresourceRange.baseMipLevel = 0;
  barrier[0].subresourceRange.levelCount = 1;
  barrier[0].subresourceRange.layerCount = 1;

  barrier[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier[1].srcQueueFamilyIndex = 0;
  barrier[1].dstQueueFamilyIndex = 0;
  barrier[1].image = vk::SWAP_CHAIN->images[globals::CURRENT_FRAME];
  barrier[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier[1].subresourceRange.baseArrayLayer = 0;
  barrier[1].subresourceRange.baseMipLevel = 0;
  barrier[1].subresourceRange.levelCount = 1;
  barrier[1].subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(
      vk::CURRENT_FRAME_COMMAND->m_commandBuffer,
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
      VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 0, nullptr, 2, barrier);
}
void VkTempLayer::onEvent(Event &event) {
  EventDispatcher dispatcher(event);
  dispatcher.dispatch<MouseButtonPressEvent>(
      SE_BIND_EVENT_FN(VkTempLayer::onMouseButtonPressEvent));
  dispatcher.dispatch<MouseButtonReleaseEvent>(
      SE_BIND_EVENT_FN(VkTempLayer::onMouseButtonReleaseEvent));
  dispatcher.dispatch<MouseMoveEvent>(
      SE_BIND_EVENT_FN(VkTempLayer::onMouseMoveEvent));
  // dispatcher.dispatch<DebugLayerChanged>(
  //    SE_BIND_EVENT_FN(VkTempLayer::onDebugLayerEvent));
  dispatcher.dispatch<WindowResizeEvent>(
      SE_BIND_EVENT_FN(VkTempLayer::onResizeEvent));
  // dispatcher.dispatch<DebugRenderConfigChanged>(
  //    SE_BIND_EVENT_FN(VkTempLayer::onDebugConfigChanged));
  // dispatcher.dispatch<ShaderCompileEvent>(
  //    SE_BIND_EVENT_FN(VkTempLayer::onShaderCompileEvent));
  // dispatcher.dispatch<ReloadScriptsEvent>(
  //    SE_BIND_EVENT_FN(VkTempLayer::onReloadScriptEvent));
}

void VkTempLayer::clear() {
  vkDeviceWaitIdle(vk::LOGICAL_DEVICE);

  // if constexpr (!USE_PUSH) {
  // vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE, m_setLayout, nullptr);
  // vkFreeMemory(vk::LOGICAL_DEVICE,m_rt.deviceMemory,nullptr);
  //}
  // TODO render target manager?
  vk::destroyFrameBuffer(vk::LOGICAL_DEVICE, m_tempFrameBuffer, m_rt);
  // vk::TEXTURE_MANAGER->free(textureHandle);
}

bool VkTempLayer::onMouseButtonPressEvent(MouseButtonPressEvent &e) {
  if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::LEFT) {
    leftDown = true;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::RIGHT) {
    rightDown = true;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::MIDDLE) {
    middleDown = true;
    return true;
  }
  return false;
}

bool VkTempLayer::onMouseButtonReleaseEvent(MouseButtonReleaseEvent &e) {
  if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::LEFT) {
    leftDown = false;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::RIGHT) {
    rightDown = false;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::MIDDLE) {
    middleDown = false;
    return true;
  }
  return false;
}

bool VkTempLayer::onMouseMoveEvent(MouseMoveEvent &e) {
  const float deltaX = previousX - e.getX();
  const float deltaY = previousY - e.getY();
  if (leftDown) {
    globals::MAIN_CAMERA->rotCamera(deltaX, deltaY);
  } else if (middleDown) {
    globals::MAIN_CAMERA->panCamera(deltaX, deltaY);
  } else if (rightDown) {
    globals::MAIN_CAMERA->zoomCamera(deltaX);
  }

  // storing old position
  previousX = e.getX();
  previousY = e.getY();
  return true;
}
/*
void removeDebugNode(DependencyGraph *graph, GNode *debugNode) {
  // disconnect input
  assert(debugNode->isOfType("FramePassDebugNode"));
  int inCount;
  const GPlug *inPlugs = debugNode->getInputPlugs(inCount);
  assert(inCount == 1);
  const ResizableVector<const GPlug *> *inConnections =
      debugNode->getPlugConnections(&inPlugs[0]);
  assert((*inConnections).size() == 1);
  const GPlug *inConnectionPlug = (*inConnections)[0];

  // removing the connection in both directions
  debugNode->removeConnection(&inPlugs[0], inConnectionPlug);
  inConnectionPlug->nodePtr->removeConnection(inConnectionPlug, &inPlugs[0]);

  // disconnect output
  int outputCount;
  const GPlug *outPlugs = debugNode->getOutputPlugs(outputCount);
  assert(outputCount == 1);
  const ResizableVector<const GPlug *> *outConnections =
      debugNode->getPlugConnections(&outPlugs[0]);
  assert((*outConnections).size() == 1);
  const GPlug *outConnectionPlug = (*outConnections)[0];
  debugNode->removeConnection(&outPlugs[0], outConnectionPlug);
  outConnectionPlug->nodePtr->removeConnection(outConnectionPlug, &outPlugs[0]);

  // now lets connect the two sides
  graph->connectNodes(inConnectionPlug, outConnectionPlug);
  graph->removeNode(debugNode);
  delete debugNode;
}
void addDebugNode(DependencyGraph *graph, GNode *debugNode) {

  assert(debugNode->isOfType("FramePassDebugNode"));
  assert(graph->getFinalNode() != nullptr);

  GNode *finalNode = graph->getFinalNode();
  // disconnect input
  int inCount;
  const GPlug *inPlugs = finalNode->getInputPlugs(inCount);
  assert(inCount == 1);
  const ResizableVector<const GPlug *> *inConnections =
      finalNode->getPlugConnections(&inPlugs[0]);
  assert((*inConnections).size() == 1);
  const GPlug *inConnectionPlug = (*inConnections)[0];
  finalNode->removeConnection(&inPlugs[0], inConnectionPlug);
  inConnectionPlug->nodePtr->removeConnection(inConnectionPlug, &inPlugs[0]);

  // no output to disconnect, final node has no output
  // now lets connect the two sides
  int debugInputCount;
  const GPlug *debugInputPlugs = debugNode->getInputPlugs(debugInputCount);
  int debugOutputCount;
  const GPlug *debugOutputPlugs = debugNode->getOutputPlugs(debugOutputCount);
  graph->connectNodes(inConnectionPlug, &debugInputPlugs[0]);

  graph->connectNodes(&debugOutputPlugs[0], &inPlugs[0]);

  graph->addNode(debugNode);
}

bool VkTempLayer::onDebugLayerEvent(DebugLayerChanged &e) {
  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
  switch (e.getLayer()) {
  case (0): {
    // if we have 0, we have no layer to debug so we can just check if there
    // there is a debug node and remove it
    const GNode *debugNode =
        dx12::RENDERING_GRAPH->findNodeOfType("FramePassDebugNode");
    if (debugNode == nullptr) { // no debug we are good
      return true;
    }

    removeDebugNode(dx12::RENDERING_GRAPH, const_cast<GNode *>(debugNode));
    dx12::RENDERING_GRAPH->finalizeGraph();
    RenderGraphChanged *graphE = new RenderGraphChanged();
    globals::APPLICATION->queueEventForEndOfFrame(graphE);
    return true;
  }
  case (1):
  case (2):
  case (3):
  case (4):
  case (5):
  case (6):
  case (7): {
    // lets add debug
    const GNode *debugNode =
        dx12::RENDERING_GRAPH->findNodeOfType("FramePassDebugNode");
    // debug already there, maybe i just need to change configuration?
    if (debugNode != nullptr) { // no debug we are good
      static_cast<FramePassDebugNode *>(const_cast<GNode *>(debugNode))
          ->setDebugIndex(e.getLayer());
      return true;
    }
    // lest add a debug node
    // TODO move the allocator inside the graph? not sure yet
    auto debug = new FramePassDebugNode(*alloc);
    debug->setDebugIndex(e.getLayer());
    addDebugNode(dx12::RENDERING_GRAPH, debug);
    dx12::RENDERING_GRAPH->finalizeGraph();
    auto *graphE = new RenderGraphChanged();
    globals::APPLICATION->queueEventForEndOfFrame(graphE);
    return true;
  }
  }
  return false;
}*/

bool VkTempLayer::onResizeEvent(WindowResizeEvent &e) {
  // need to recreate the frame buffer, this is temporary
  vk::destroyFrameBuffer(vk::LOGICAL_DEVICE, m_tempFrameBuffer, m_rt);
  createRenderTargetAndFrameBuffer(globals::ENGINE_CONFIG->m_windowWidth,
                                   globals::ENGINE_CONFIG->m_windowHeight);

  return true;
}

void VkTempLayer::setupCameraForFrame() {}

/*
bool VkTempLayer::onDebugConfigChanged(DebugRenderConfigChanged &e) {
  GNode *debugNode =
      dx12::RENDERING_GRAPH->findNodeOfType("FramePassDebugNode");
  if (debugNode) {
    auto *debugNodeTyped = (FramePassDebugNode *)debugNode;
    debugNodeTyped->setConfig(e.getConfig());
  }
  return true;
}
bool VkTempLayer::onShaderCompileEvent(ShaderCompileEvent &e) {
  SE_CORE_INFO("Reading to compile shader");
  dx12::PSO_MANAGER->recompilePSOFromShader(e.getShader(), e.getOffsetPath());
  return true;
}

bool VkTempLayer::onReloadScriptEvent(ReloadScriptsEvent &) {
  globals::SCRIPTING_CONTEXT->reloadContext();
  return true;
}*/
} // namespace SirEngine
