#include "SirEngine/layers/graphics3DLayer.h"
#include "SirEngine/application.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/events/debugEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"
#include "SirEngine/graphics/nodeGraph.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include <DirectXMath.h>

#include "SirEngine/events/renderGraphEvent.h"
#include "SirEngine/graphics/nodes/DebugNode.h"
#include "SirEngine/graphics/nodes/FinalBlitNode.h"
#include "SirEngine/graphics/nodes/assetManagerNode.h"
#include "SirEngine/graphics/nodes/deferredLighting.h"
#include "SirEngine/graphics/nodes/gbufferPass.h"
#include "SirEngine/graphics/nodes/proceduralSkybox.h"
#include "SirEngine/graphics/postProcess/effects/gammaAndToneMappingEffect.h"
#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include "SirEngine/graphics/renderingContext.h"

namespace SirEngine {

void Graphics3DLayer::onAttach() {
  globals::MAIN_CAMERA = new Camera3DPivot();
  // globals::MAIN_CAMERA->setLookAt(0, 125, 0);
  // globals::MAIN_CAMERA->setPosition(00, 125, 60);

  globals::MAIN_CAMERA->setLookAt(0, 0, 0);
  globals::MAIN_CAMERA->setPosition(00, 0, 5);
  globals::MAIN_CAMERA->updateCamera();

  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;

  if (!currentFc->isListOpen) {
    dx12::resetAllocatorAndList(currentFc);
  }

  sphereH = globals::ASSET_MANAGER->loadAsset("data/assets/sphere.json");
  // globals::ASSET_MANAGER->loadAsset("data/assets/plane.json");
  dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE, currentFc);
  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);

  dx12::RENDERING_GRAPH = new Graph();

  auto assetNode = new AssetManagerNode();
  auto finalBlit = new FinalBlitNode();
  // auto simpleForward = new SimpleForward("simpleForward");
  auto postProcess = new PostProcessStack();
  auto gbufferPass = new GBufferPass("GBufferPass");
  auto lighting = new DeferredLightingPass("Deferred lighting");
  auto sky = new ProceduralSkyBoxPass("Procedural Sky");
  // auto bw  =
  auto gamma = postProcess->allocateRenderPass<GammaAndToneMappingEffect>(
      "GammaToneMapping");
  postProcess->initialize();

  // temporary graph for testing
  dx12::RENDERING_GRAPH->addNode(assetNode);
  dx12::RENDERING_GRAPH->addNode(finalBlit);
  // dx12::RENDERING_GRAPH->addNode(simpleForward);
  dx12::RENDERING_GRAPH->addNode(gbufferPass);
  dx12::RENDERING_GRAPH->addNode(lighting);
  dx12::RENDERING_GRAPH->addNode(sky);
  dx12::RENDERING_GRAPH->setFinalNode(finalBlit);

  dx12::RENDERING_GRAPH->connectNodes(assetNode, "matrices", gbufferPass,
                                      "matrices");
  dx12::RENDERING_GRAPH->connectNodes(assetNode, "meshes", gbufferPass,
                                      "meshes");
  dx12::RENDERING_GRAPH->connectNodes(assetNode, "materials", gbufferPass,
                                      "materials");

  // auto bw = new DebugNode("debugBW");
  dx12::RENDERING_GRAPH->addNode(postProcess);
  // dx12::RENDERING_GRAPH->connectNodes(simpleForward, "outTexture",
  // postProcess,
  //                                    "inTexture");

  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, "geometry", lighting,
                                      "geometry");
  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, "normal", lighting,
                                      "normal");
  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, "specular", lighting,
                                      "specular");
  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, "depth", lighting, "depth");

  dx12::RENDERING_GRAPH->connectNodes(lighting, "lighting", sky,
                                      "fullscreenPass");
  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, "depth", sky, "depth");

  dx12::RENDERING_GRAPH->connectNodes(sky, "buffer", postProcess, "inTexture");

  dx12::RENDERING_GRAPH->connectNodes(postProcess, "outTexture", finalBlit,
                                      "inTexture");

  dx12::RENDERING_GRAPH->finalizeGraph();
}
void Graphics3DLayer::onDetach() {}
void Graphics3DLayer::onUpdate() {

  // setting up camera for the frame
  globals::CONSTANT_BUFFER_MANAGER->processBufferedData();
  globals::RENDERING_CONTEX->setupCameraForFrame();
  // evaluating rendering graph
  dx12::RENDERING_GRAPH->compute();

  // making any clean up for the mesh manager if we have to
  dx12::MESH_MANAGER->clearUploadRequests();
}
void Graphics3DLayer::onEvent(Event &event) {

  EventDispatcher dispatcher(event);
  dispatcher.dispatch<MouseButtonPressEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onMouseButtonPressEvent));
  dispatcher.dispatch<MouseButtonReleaseEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onMouseButtonReleaseEvent));
  dispatcher.dispatch<MouseMoveEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onMouseMoveEvent));
  dispatcher.dispatch<DebugLayerChanged>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onDebugLayerEvent));
  dispatcher.dispatch<WindowResizeEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onResizeEvent));
  dispatcher.dispatch<DebugRenderConfigChanged>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onDebugDepthChanged));
}

void Graphics3DLayer::clear() {}

bool Graphics3DLayer::onMouseButtonPressEvent(MouseButtonPressEvent &e) {
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

bool Graphics3DLayer::onMouseButtonReleaseEvent(MouseButtonReleaseEvent &e) {
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

bool Graphics3DLayer::onMouseMoveEvent(MouseMoveEvent &e) {
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

bool Graphics3DLayer::onDebugLayerEvent(DebugLayerChanged &e) {
  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
  switch (e.getLayer()) {
  case (0): {
    // if we have 0, we have no layer to debug so we can just check if there
    // there is a debug node and remove it
    GraphNode *debugNode = dx12::RENDERING_GRAPH->findNodeOfType("DebugNode");
    if (debugNode == nullptr) { // no debug we are good
      return true;
    }
    dx12::RENDERING_GRAPH->removeDebugNode(debugNode);
    dx12::RENDERING_GRAPH->finalizeGraph();
    RenderGraphChanged *graphE = new RenderGraphChanged();
    globals::APPLICATION->queueEventForEndOfFrame(graphE);
    return true;
  }
  case (1):
  case (2):
  case (3):
  case (4): {
    // lets add debug
    GraphNode *debugNode = dx12::RENDERING_GRAPH->findNodeOfType("DebugNode");
    // debug already there, maybe i just need to change configuration?
    if (debugNode != nullptr) { // no debug we are good
      static_cast<DebugNode *>(debugNode)->setDebugIndex(e.getLayer());
      return true;
    }
    // lest add a debug node
    auto debug = new DebugNode("DebugNode");
    debug->setDebugIndex(e.getLayer());
    dx12::RENDERING_GRAPH->addDebugNode(debug);
    dx12::RENDERING_GRAPH->finalizeGraph();
    RenderGraphChanged *graphE = new RenderGraphChanged();
    globals::APPLICATION->queueEventForEndOfFrame(graphE);
    return true;
  }
  }
  return false;
}

bool Graphics3DLayer::onResizeEvent(WindowResizeEvent &e) {
  // propagate the resize to every node of the graph
  dx12::RENDERING_GRAPH->resize(e.getWidth(), e.getHeight());
  return true;
}

bool Graphics3DLayer::onDebugDepthChanged(DebugRenderConfigChanged &e) {
  GraphNode *debugNode = dx12::RENDERING_GRAPH->findNodeOfType("DebugNode");
  if (debugNode) {
    auto *debugNodeTyped = (DebugNode *)debugNode;
    debugNodeTyped->setConfig(e.getConfig());
  }
  return true;
}
} // namespace SirEngine
