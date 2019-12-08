#include "SirEngine/application.h"
#include "SirEngine/events/debugEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"

#include "SirEngine/engineConfig.h"
#include "SirEngine/layers/vkTempLayer.h"
#include "platform/windows/graphics/vk/graphicsPipeline.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkDescriptors.h"
#include "platform/windows/graphics/vk/vkLoad.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "platform/windows/graphics/vk/vkShaderManager.h"
#include "platform/windows/graphics/vk/vkSwapChain.h"
#include "platform/windows/graphics/vk/volk.h"

namespace SirEngine {

void VkTempLayer::onAttach() {
  globals::MAIN_CAMERA = new Camera3DPivot();
  // globals::MAIN_CAMERA->setLookAt(0, 125, 0);
  // globals::MAIN_CAMERA->setPosition(00, 125, 60);

  globals::MAIN_CAMERA->setLookAt(0, 14, 0);
  globals::MAIN_CAMERA->setPosition(0, 14, 10);
  globals::MAIN_CAMERA->updateCamera();

  vk::initStaticSamplers();
  // if constexpr (!USE_PUSH) {
  vk::createDescriptorPool(vk::LOGICAL_DEVICE, {10000, 10000}, m_dPool);
  //}

  vk::createStaticSamplerDescriptorSet(m_dPool, m_samplersDescriptorSets,
                                       m_samplersLayout);
  m_vs = vk::SHADER_MANAGER->getShaderFromName("triangleVS");
  m_fs = vk::SHADER_MANAGER->getShaderFromName("trianglePS");
  assert(m_vs);
  assert(m_fs);

  // load mesh
  loadMesh("../data/external/vk/lucy.obj", m_mesh);

  // allocate memory buffer for the mesh
  VkPhysicalDeviceMemoryProperties memoryRequirements;
  vkGetPhysicalDeviceMemoryProperties(vk::PHYSICAL_DEVICE, &memoryRequirements);
  createBuffer(
      m_vertexBuffer, vk::LOGICAL_DEVICE, memoryRequirements, 128 * 1024 * 1024,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  createBuffer(
      m_indexBuffer, vk::LOGICAL_DEVICE, memoryRequirements, 128 * 1024 * 1024,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  assert(m_vertexBuffer.size >= m_mesh.vertices.size() * sizeof(vk::Vertex));
  assert(m_indexBuffer.size >= m_mesh.indices.size() * sizeof(uint32_t));

  memcpy(m_vertexBuffer.data, m_mesh.vertices.data(),
         m_mesh.vertices.size() * sizeof(vk::Vertex));
  memcpy(m_indexBuffer.data, m_mesh.indices.data(),
         m_mesh.indices.size() * sizeof(uint32_t));

  SET_DEBUG_NAME(m_vertexBuffer.buffer, VK_OBJECT_TYPE_BUFFER, "vertex buffer");
  SET_DEBUG_NAME(m_indexBuffer.buffer, VK_OBJECT_TYPE_BUFFER, "index buffer");

  m_pipeline =
      vk::createGraphicsPipeline(vk::LOGICAL_DEVICE, m_vs, m_fs,
                                 vk::RENDER_PASS, nullptr, m_samplersLayout);

  loadTextureFromFile("../data/external/vk/uv.DDS",
                      VK_FORMAT_BC1_RGBA_UNORM_BLOCK, vk::LOGICAL_DEVICE,
                      uvTexture);

  // if constexpr (!USE_PUSH) {
  createDescriptorLayoutAdvanced();
  //}

  // vk::PSO_MANAGER->loadRawPSO("../data/pso/forwardPhongPSO.json");
}
void init_sampler(VkSampler &sampler) {

  VkSamplerCreateInfo samplerCreateInfo = {};
  samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
  samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.mipLodBias = 0.0;
  samplerCreateInfo.anisotropyEnable = VK_FALSE;
  samplerCreateInfo.maxAnisotropy = 1;
  samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
  samplerCreateInfo.minLod = 0.0;
  samplerCreateInfo.maxLod = 0.0;
  samplerCreateInfo.compareEnable = VK_FALSE;
  samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

  /* create sampler */
  VK_CHECK(
      vkCreateSampler(vk::LOGICAL_DEVICE, &samplerCreateInfo, NULL, &sampler));
}

void VkTempLayer::createDescriptorLayoutAdvanced() {

  constexpr int resource_count = 2;
  VkDescriptorSetLayoutBinding resource_binding[resource_count] = {};
  resource_binding[0].binding = 0;
  resource_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  resource_binding[0].descriptorCount = 1;
  resource_binding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  resource_binding[0].pImmutableSamplers = NULL;
  resource_binding[1].binding = 1;
  resource_binding[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  resource_binding[1].descriptorCount = 1;
  resource_binding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  resource_binding[1].pImmutableSamplers = NULL;

  VkDescriptorSetLayoutCreateInfo resource_layout_info[1] = {};
  resource_layout_info[0].sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  resource_layout_info[0].pNext = NULL;
  resource_layout_info[0].bindingCount = resource_count;
  resource_layout_info[0].pBindings = resource_binding;

  VK_CHECK(vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, resource_layout_info,
                                       NULL, &m_setLayout));

  SET_DEBUG_NAME(m_setLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                 "resourceBindingsLayoutNoSamplers");

  // Allocates an empty descriptor set without actual descriptors from the pool
  // using the set layout
  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = m_dPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &m_setLayout; // the layout we defined for the set,
                                           // so it also knows the size
  VK_CHECK(vkAllocateDescriptorSets(vk::LOGICAL_DEVICE, &allocateInfo,
                                    &m_meshDescriptorSet));

  // Create our separate sampler
  init_sampler(separateSampler);

  samplerInfo.sampler = separateSampler;

  // Update the descriptor set with the actual descriptors matching shader
  // bindings set in the layout
  // this far we defined just what descriptor we wanted and how they were setup,
  // now we need to actually define the content of those descriptrs, the actual
  // resources
  VkWriteDescriptorSet writeDescriptorSets[2] = {};

  // actual information of the descriptor, in this case it is our mesh buffer
  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = m_vertexBuffer.buffer;
  bufferInfo.offset = 0;
  bufferInfo.range = m_vertexBuffer.size;

  // updating descriptors, with no samplers bound
  // Binding 0: Object matrices Uniform buffer
  writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSets[0].dstSet = m_meshDescriptorSet;
  writeDescriptorSets[0].dstBinding = 0;
  writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writeDescriptorSets[0].pBufferInfo = &bufferInfo;
  writeDescriptorSets[0].descriptorCount = 1;

  // Binding 1: Object texture
  writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSets[1].dstSet = m_meshDescriptorSet;
  writeDescriptorSets[1].dstBinding = 1;
  writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  // Images use a different descriptor strucutre, so we use pImageInfo instead
  // of pBufferInfo
  writeDescriptorSets[1].pImageInfo = &uvTexture.descriptor;
  writeDescriptorSets[1].descriptorCount = 1;

  VkWriteDescriptorSet samplersWrite[1] = {};

  // Execute the writes to update descriptors for this set
  // Note that it's also possible to gather all writes and only run updates
  // once, even for multiple sets This is possible because each
  // VkWriteDescriptorSet also contains the destination set to be updated
  // For simplicity we will update once per set instead
  // vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, ARRAYSIZE(writeDescriptorSets),
  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, ARRAYSIZE(writeDescriptorSets),
                         writeDescriptorSets, 0, nullptr);
}

void VkTempLayer::onDetach() {}
void VkTempLayer::onUpdate() {
  static float step = 0.01f;
  static int index = 0;
  static int counter = 0;

  static VkClearColorValue color{0, 0, 0, 1};
  color.float32[index] += 1.0f / 256.0f;
  counter++;
  if (counter == 255) {
    index += 1;
    counter = 0;
  }
  if (index == 3) {
    counter = 0;
    index = 0;
    color.uint32[0] = 0;
    color.uint32[1] = 0;
    color.uint32[2] = 0;
  }

  // lets us start a render pass

  VkClearValue clear{};
  clear.color = color;

  VkRenderPassBeginInfo beginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  beginInfo.renderPass = vk::RENDER_PASS;
  beginInfo.framebuffer = vk::SWAP_CHAIN->frameBuffers[globals::CURRENT_FRAME];

  // similar to a viewport mostly used on "tiled renderers" to optimize, talking
  // about hardware based tile renderer, aka mobile GPUs.
  beginInfo.renderArea.extent.width =
      static_cast<int32_t>(globals::ENGINE_CONFIG->m_windowWidth);
  beginInfo.renderArea.extent.height =
      static_cast<int32_t>(globals::ENGINE_CONFIG->m_windowHeight);
  beginInfo.clearValueCount = 1;
  beginInfo.pClearValues = &clear;

  vkCmdBeginRenderPass(vk::COMMAND_BUFFER, &beginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

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
  vkCmdSetViewport(vk::COMMAND_BUFFER, 0, 1, &viewport);
  vkCmdSetScissor(vk::COMMAND_BUFFER, 0, 1, &scissor);
  vkCmdBindPipeline(vk::COMMAND_BUFFER, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_pipeline);

  /*
  if constexpr (USE_PUSH) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_vertexBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = m_vertexBuffer.size;

    VkWriteDescriptorSet descriptor[1]{};
    descriptor[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor[0].dstBinding = 0;
    descriptor[0].descriptorCount = 1;
    descriptor[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptor[0].pBufferInfo = &bufferInfo;

    vkCmdPushDescriptorSetKHR(COMMAND_BUFFER, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              PIPELINE_LAYOUT, 0, ARRAYSIZE(descriptor),
                              descriptor);
  } else {
  */
  VkDescriptorSet sets[] = {m_meshDescriptorSet, m_samplersDescriptorSets};
  // multiple descriptor sets
  vkCmdBindDescriptorSets(vk::COMMAND_BUFFER, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          vk::PIPELINE_LAYOUT, 0, 2, sets, 0, nullptr);
  // vkCmdBindDescriptorSets(vk::COMMAND_BUFFER,
  // VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                        vk::PIPELINE_LAYOUT, 0, 1,
  //                        &m_samplersDescriptorSets, 0, nullptr);

  ////single descriptor set
  // vkCmdBindDescriptorSets(vk::COMMAND_BUFFER,
  // VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                        vk::PIPELINE_LAYOUT, 0, 1, &m_meshDescriptorSet, 0,
  //                        nullptr);

  // vkCmdBindDescriptorSets(vk::COMMAND_BUFFER,
  // VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                        vk::PIPELINE_LAYOUT, 0, 1,
  //                        &m_samplersDescriptorSets, 1, nullptr);

  vkCmdBindIndexBuffer(vk::COMMAND_BUFFER, m_indexBuffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);
  // vkCmdDraw(COMMAND_BUFFER, 3, 1, 0, 0);
  vkCmdDrawIndexed(vk::COMMAND_BUFFER, static_cast<uint32_t>(m_mesh.indices.size()), 1, 0, 0, 0);

  vkCmdEndRenderPass(vk::COMMAND_BUFFER);
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
  vkDestroyShaderModule(vk::LOGICAL_DEVICE, m_vs, nullptr);
  vkDestroyShaderModule(vk::LOGICAL_DEVICE, m_fs, nullptr);
  destroyBuffer(vk::LOGICAL_DEVICE, m_vertexBuffer);
  destroyBuffer(vk::LOGICAL_DEVICE, m_indexBuffer);
  for (auto layout : vk::LAYOUTS_TO_DELETE) {
    vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE, layout, nullptr);
  }
  // if constexpr (!USE_PUSH) {
  vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE, m_setLayout, nullptr);
  vkDestroyDescriptorPool(vk::LOGICAL_DEVICE, m_dPool, nullptr);
  //}
  destroyTexture(vk::LOGICAL_DEVICE, uvTexture);
  vkDestroyPipeline(vk::LOGICAL_DEVICE, m_pipeline, nullptr);
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
  assert(0);
  // propagate the resize to every node of the graph
  // dx12::RENDERING_GRAPH->onResizeEvent(e.getWidth(), e.getHeight());
  return true;
}

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
