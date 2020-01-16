#include "SirEngine/layers/vkTempLayer.h"

#include "SirEngine/application.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/bufferManager.h"
#include "SirEngine/events/debugEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"
#include "SirEngine/graphics/debugRenderer.h"
#include "SirEngine/graphics/nodes/FinalBlitNode.h"
#include "SirEngine/graphics/nodes/debugDrawNode.h"
#include "SirEngine/graphics/nodes/vkSimpleForward.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/materialManager.h"

namespace SirEngine {
void VkTempLayer::initGrass() {
  // lets read the grass file
  const char *grassFile = "../data/external/grass/points.json";
  auto jobj = getJsonObj(grassFile);

  // compute the amount of memory needed
  const uint32_t tileCount = jobj.size();
  assert((tileCount > 0) && "no tiles found in the file");
  // assuming all tiles have same size
  assertInJson(jobj[0], "points");
  const uint32_t pointCount = jobj[0]["points"].size();
  const uint64_t totalSize = sizeof(float) * 4 * pointCount * tileCount;

  auto *data =
      reinterpret_cast<float *>(globals::FRAME_ALLOCATOR->allocate(totalSize));

  auto *aabbs = reinterpret_cast<BoundingBox *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(BoundingBox) * tileCount));

  // looping the tiles
  int counter = 0;
  int tileCounter = 0;
  for (const auto &tile : jobj) {
    assertInJson(tile, "aabb");
    assertInJson(tile, "points");
    const auto &aabbJ = tile["aabb"];
    const glm::vec3 minAABB(aabbJ[0][0].get<float>(), aabbJ[0][1].get<float>(),
                            aabbJ[0][2].get<float>());
    const glm::vec3 maxAABB(aabbJ[1][0].get<float>(), aabbJ[1][1].get<float>(),
                            aabbJ[1][2].get<float>());
    aabbs[tileCounter].min = minAABB;
    aabbs[tileCounter].max = maxAABB;

    const auto &points = tile["points"];
    uint32_t currentPointCount = points.size();
    assert(currentPointCount == pointCount);

    for (uint32_t i = 0; i < currentPointCount; ++i) {
      data[counter++] = points[i][0].get<float>();
      data[counter++] = points[i][1].get<float>();
      data[counter++] = points[i][2].get<float>();
      data[counter++] = 1;
    }
    tileCounter++;
  }

  m_debugHandle = globals::DEBUG_RENDERER->drawBoundingBoxes(
      aabbs, tileCount, glm::vec4(1, 0, 0, 1), "debugGrassTiles");
  // we have the tiles, good now we want to render the blades
  m_grassBuffer = globals::BUFFER_MANAGER->allocate(
      totalSize, data, "grassBuffer", pointCount * tileCount, sizeof(float) * 4,
      BufferManager::STORAGE_BUFFER);
  // setup a material

  const char *queues[5] = {"grassForward", nullptr, nullptr, nullptr, nullptr};

  m_grassMaterial = globals::MATERIAL_MANAGER->allocateMaterial(
      "grassMaterial", MaterialManager::ALLOCATE_MATERIAL_FLAG_BITS::NONE,
      queues);
  globals::MATERIAL_MANAGER->bindBuffer(m_grassMaterial, m_grassBuffer, 0,
                                        SHADER_QUEUE_FLAGS::FORWARD);

  //lets create the needed stuff to add the object to the queue 
  RenderableDescription description{};
  description.buffer = m_grassBuffer;
  description.subranges[0].m_offset = 0;
  description.subranges[0].m_size = totalSize;
  description.subragesCount = 1;

  description.materialHandle = m_grassMaterial; 
  //*3 because we render a triangle for now
  description.primitiveToRender = pointCount*tileCount*3;
  globals::RENDERING_CONTEXT->addRenderablesToQueue(description);

	
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

  initGrass();

  alloc =
      new GraphAllocators{globals::STRING_POOL, globals::PERSISTENT_ALLOCATOR};

  globals::RENDERING_GRAPH = new DependencyGraph();
  const auto forward = new VkSimpleForward(*alloc);
  const auto debugDraw = new DebugDrawNode(*alloc);
  const auto finalBlit = new FinalBlitNode(*alloc);

  // temporary graph for testing
  globals::RENDERING_GRAPH->addNode(forward);
  globals::RENDERING_GRAPH->addNode(debugDraw);
  globals::RENDERING_GRAPH->addNode(finalBlit);
  globals::RENDERING_GRAPH->setFinalNode(finalBlit);

  globals::RENDERING_GRAPH->connectNodes(forward, VkSimpleForward::OUT_TEXTURE,
                                         debugDraw, DebugDrawNode::IN_TEXTURE);
  globals::RENDERING_GRAPH->connectNodes(forward, VkSimpleForward::DEPTH_RT,
                                         debugDraw, DebugDrawNode::DEPTH_RT);
  globals::RENDERING_GRAPH->connectNodes(debugDraw, DebugDrawNode::OUT_TEXTURE,
                                         finalBlit, FinalBlitNode::IN_TEXTURE);

  // TODO this whole reset execute flush needs to be reworked
  globals::RENDERING_CONTEXT->resetGlobalCommandList();
  globals::RENDERING_GRAPH->finalizeGraph();
  globals::RENDERING_CONTEXT->executeGlobalCommandList();
  globals::RENDERING_CONTEXT->flush();
}

void VkTempLayer::onDetach() {}
void VkTempLayer::onUpdate() {
  globals::RENDERING_CONTEXT->setupCameraForFrame();
  // evaluating rendering graph
  globals::RENDERING_GRAPH->compute();

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
  //}
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
  // vkDeviceWaitIdle(vk::LOGICAL_DEVICE);
  globals::RENDERING_GRAPH->clear();
  globals::DEBUG_RENDERER->free(m_debugHandle);
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
  // TODO re-add resizing
  // createRenderTargetAndFrameBuffer(globals::ENGINE_CONFIG->m_windowWidth,
  //                                 globals::ENGINE_CONFIG->m_windowHeight);

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
}  // namespace SirEngine
