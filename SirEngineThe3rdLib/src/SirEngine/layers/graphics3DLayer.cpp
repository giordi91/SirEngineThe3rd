#include "SirEngine/layers/graphics3DLayer.h"
#include "SirEngine/animation/animationManager.h"
#include "SirEngine/animation/animationPlayer.h"
#include "SirEngine/animation/skeleton.h"
#include "SirEngine/application.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/events/debugEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"
#include "SirEngine/graphics/nodeGraph.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/debugRenderer.h"
#include <DirectXMath.h>

#include "SirEngine/events/renderGraphEvent.h"
#include "SirEngine/events/shaderCompileEvent.h"
#include "SirEngine/graphics/nodes/FinalBlitNode.h"
#include "SirEngine/graphics/nodes/assetManagerNode.h"
#include "SirEngine/graphics/nodes/debugDrawNode.h"
#include "SirEngine/graphics/nodes/deferredLighting.h"
#include "SirEngine/graphics/nodes/framePassDebugNode.h"
#include "SirEngine/graphics/nodes/gbufferPassPBR.h"
#include "SirEngine/graphics/nodes/simpleForward.h"
#include "SirEngine/graphics/nodes/skybox.h"
#include "SirEngine/graphics/postProcess/effects/SSSSSEffect.h"
#include "SirEngine/graphics/postProcess/effects/gammaAndToneMappingEffect.h"
#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include "SirEngine/graphics/renderingContext.h"

#include "SirEngine/skinClusterManager.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/bufferManagerDx12.h"

#include "SirEngine/scripting/scriptingContext.h"
#include <SirEngine/events/scriptingEvent.h>

namespace SirEngine {

void Graphics3DLayer::onAttach() {
  globals::MAIN_CAMERA = new Camera3DPivot();
  // globals::MAIN_CAMERA->setLookAt(0, 125, 0);
  // globals::MAIN_CAMERA->setPosition(00, 125, 60);

  globals::MAIN_CAMERA->setLookAt(0, 14, 0);
  globals::MAIN_CAMERA->setPosition(0, 14, 10);
  globals::MAIN_CAMERA->updateCamera();

  //globals::MAIN_CAMERA->setLookAt(0, 0, 0);
  //globals::MAIN_CAMERA->setPosition(0, 0, 1);
  //globals::MAIN_CAMERA->updateCamera();

  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;

  if (!currentFc->isListOpen) {
    dx12::resetAllocatorAndList(currentFc);
  }

  globals::ASSET_MANAGER->loadScene(globals::START_SCENE_PATH);
  dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE, currentFc);
  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);

  alloc =
      new GraphAllocators{globals::STRING_POOL, globals::PERSISTENT_ALLOCATOR};
  dx12::RENDERING_GRAPH = new DependencyGraph();
  const auto assetNode = new AssetManagerNode(*alloc);
  const auto finalBlit = new FinalBlitNode(*alloc);
  const auto simpleForward = new SimpleForward(*alloc);
  auto postProcess = new PostProcessStack(*alloc);
  // auto gbufferPass = new GBufferPass("GBufferPass");
  const auto gbufferPass = new GBufferPassPBR(*alloc);
  const auto lighting = new DeferredLightingPass(*alloc);
  // auto sky = new ProceduralSkyBoxPass("Procedural Sky");
  const auto sky = new SkyBoxPass(*alloc);
  const auto debugDraw = new DebugDrawNode(*alloc);

  postProcess->allocateRenderPass<SSSSSEffect>("SSSSS");
  postProcess->allocateRenderPass<GammaAndToneMappingEffect>(
      "GammaToneMapping");
  postProcess->initialize();

  // temporary graph for testing
  dx12::RENDERING_GRAPH->addNode(assetNode);
  dx12::RENDERING_GRAPH->addNode(finalBlit);
  dx12::RENDERING_GRAPH->addNode(gbufferPass);
  dx12::RENDERING_GRAPH->addNode(lighting);
  dx12::RENDERING_GRAPH->addNode(sky);
  dx12::RENDERING_GRAPH->addNode(simpleForward);
  dx12::RENDERING_GRAPH->addNode(postProcess);
  dx12::RENDERING_GRAPH->addNode(debugDraw);
  dx12::RENDERING_GRAPH->setFinalNode(finalBlit);

  dx12::RENDERING_GRAPH->connectNodes(assetNode, AssetManagerNode::ASSET_STREAM,
                                      gbufferPass,
                                      GBufferPassPBR::ASSET_STREAM);

  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, GBufferPassPBR::GEOMETRY_RT,
                                      lighting,
                                      DeferredLightingPass::GEOMETRY_RT);
  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, GBufferPassPBR::NORMALS_RT,
                                      lighting,
                                      DeferredLightingPass::NORMALS_RT);
  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, GBufferPassPBR::SPECULAR_RT,
                                      lighting,
                                      DeferredLightingPass::SPECULAR_RT);
  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, GBufferPassPBR::DEPTH_RT,
                                      lighting, DeferredLightingPass::DEPTH_RT);

  dx12::RENDERING_GRAPH->connectNodes(
      lighting, DeferredLightingPass::LIGHTING_RT, sky, SkyBoxPass::IN_TEXTURE);

  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, GBufferPassPBR::DEPTH_RT,
                                      sky, SkyBoxPass::DEPTH);

  // connecting forward
  dx12::RENDERING_GRAPH->connectNodes(sky, SkyBoxPass::OUT_TEX, simpleForward,
                                      SimpleForward::IN_TEXTURE);
  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, GBufferPassPBR::DEPTH_RT,
                                      simpleForward, SimpleForward::DEPTH_RT);

  // dx12::RENDERING_GRAPH->connectNodes(simpleForward,
  // SimpleForward::OUT_TEXTURE,
  //                                    postProcess, "inTexture");
  dx12::RENDERING_GRAPH->connectNodes(assetNode, AssetManagerNode::ASSET_STREAM,
                                      simpleForward,
                                      SimpleForward::ASSET_STREAM);
  dx12::RENDERING_GRAPH->connectNodes(simpleForward, SimpleForward::OUT_TEXTURE,
                                      postProcess,
                                      PostProcessStack::IN_TEXTURE);

  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, GBufferPassPBR::DEPTH_RT,
                                      postProcess, PostProcessStack::DEPTH_RT);

  dx12::RENDERING_GRAPH->connectNodes(postProcess,
                                      PostProcessStack::OUT_TEXTURE, debugDraw,
                                      DebugDrawNode::IN_TEXTURE);
  dx12::RENDERING_GRAPH->connectNodes(gbufferPass, GBufferPassPBR::DEPTH_RT,
                                      debugDraw, DebugDrawNode::DEPTH_RT);

  dx12::RENDERING_GRAPH->connectNodes(debugDraw, DebugDrawNode::OUT_TEXTURE,
                                      finalBlit, FinalBlitNode::IN_TEXTURE);

  dx12::RENDERING_GRAPH->finalizeGraph();

  if (!currentFc->isListOpen) {
    dx12::resetAllocatorAndList(currentFc);
  }

  // TODO REMOVE THIS
  // const auto m_animation = globals::ANIMATION_MANAGER->loadAnimationConfig(
  //    "../data/external/animation/exported/clip/knightBIdleConfig.json");
  // auto m_animation2 = globals::ANIMATION_MANAGER->loadAnimationConfig(
  //    "../data/external/animation/exported/clip/knightBIdleConfig.json");
  // m_config = globals::ANIMATION_MANAGER->getConfig(m_animation);
  // globals::ANIMATION_MANAGER->registerState(m_config.m_anim_state);

  // dx12::DEBUG_RENDERER->drawSkeleton(m_config.m_skeleton,
  //                                   DirectX::XMFLOAT4(0, 1, 0, 1), 0.05f);

  dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE, currentFc);
  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);

  auto bboxes = dx12::MESH_MANAGER->getBoundingBoxes();
  // dx12::DEBUG_RENDERER->drawBoundingBoxes(bboxes.data(), 1,
  dx12::DEBUG_RENDERER->drawBoundingBoxes(bboxes.data(),
                                          static_cast<int>(bboxes.size()),
                                          DirectX::XMFLOAT4{1, 1, 1, 1}, "");
  // globals::SCRIPTING_CONTEXT->loadScript("../data/scripts/test.lua",true);

  // TEMP
  // animation is up to date, we can update the scene bounding boxes
  globals::RENDERING_CONTEXT->updateSceneBoundingBox();
  // draw the scene bounding box
  BoundingBox aabb = globals::RENDERING_CONTEXT->getBoundingBox();
  dx12::DEBUG_RENDERER->drawBoundingBoxes(&aabb, 1,
                                          DirectX::XMFLOAT4(0, 1, 0, 1), "");

  auto light = globals::RENDERING_CONTEXT->getLightData();
  dx12::DEBUG_RENDERER->drawMatrix(light.localToWorld, 3.0f,
                                   DirectX::XMFLOAT4(1, 0, 0, 1), "");
  dx12::DEBUG_RENDERER->drawMatrix(globals::MAIN_CAMERA->getViewInverse(DirectX::XMMatrixIdentity()), 3.0f,
                                   DirectX::XMFLOAT4(1, 0, 0, 1), "");
}
void Graphics3DLayer::onDetach() {}
void Graphics3DLayer::onUpdate() {

  globals::SCRIPTING_CONTEXT->runScriptSlot(SCRIPT_CALLBACK_SLOT::PRE_ANIM);
  globals::ANIMATION_MANAGER->evaluate();

  // update the camera position
  const AnimationConfigHandle charHandle =
      globals::ANIMATION_MANAGER->getConfigHandleFromName("knightBSkin");
  AnimationPlayer *player =
      globals::ANIMATION_MANAGER->getAnimationPlayer(charHandle);
  SkeletonPose *playerPose = player->getOutPose();
  DirectX::XMMATRIX root = playerPose->m_worldMat[0];
  // TODO manipulate camera to follow

  // animation is up to date, we can update the scene bounding boxes
  globals::RENDERING_CONTEXT->updateSceneBoundingBox();
  globals::RENDERING_CONTEXT->updateDirectionalLightMatrix();

  // upload skinning matrices
  globals::SKIN_MANAGER->uploadDirtyMatrices();

  // setting up camera for the frame
  globals::CONSTANT_BUFFER_MANAGER->processBufferedData();
  globals::RENDERING_CONTEXT->setupCameraForFrame();
  // evaluating rendering graph
  dx12::RENDERING_GRAPH->compute();

  // m_animHandle = dx12::DEBUG_RENDERER->drawAnimatedSkeleton(
  //    m_animHandle, m_config.m_anim_state, DirectX::XMFLOAT4{1, 0, 0, 1},
  //    0.1f);

  // making any clean up for the mesh manager if we have to
  dx12::CONSTANT_BUFFER_MANAGER->clearUpQueueFree();
  dx12::BUFFER_MANAGER->clearUploadRequests();
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
      SE_BIND_EVENT_FN(Graphics3DLayer::onDebugConfigChanged));
  dispatcher.dispatch<ShaderCompileEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onShaderCompileEvent));
  dispatcher.dispatch<ReloadScriptsEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onReloadScriptEvent));
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

bool Graphics3DLayer::onDebugLayerEvent(DebugLayerChanged &e) {
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
}

bool Graphics3DLayer::onResizeEvent(WindowResizeEvent &e) {
  // propagate the resize to every node of the graph
  dx12::RENDERING_GRAPH->onResizeEvent(e.getWidth(), e.getHeight());
  return true;
}

bool Graphics3DLayer::onDebugConfigChanged(DebugRenderConfigChanged &e) {
  GNode *debugNode =
      dx12::RENDERING_GRAPH->findNodeOfType("FramePassDebugNode");
  if (debugNode) {
    auto *debugNodeTyped = (FramePassDebugNode *)debugNode;
    debugNodeTyped->setConfig(e.getConfig());
  }
  return true;
}
bool Graphics3DLayer::onShaderCompileEvent(ShaderCompileEvent &e) {
  SE_CORE_INFO("Reading to compile shader");
  dx12::PSO_MANAGER->recompilePSOFromShader(e.getShader(), e.getOffsetPath());
  return true;
}

bool Graphics3DLayer::onReloadScriptEvent(ReloadScriptsEvent &) {
  globals::SCRIPTING_CONTEXT->reloadContext();
  return true;
}
} // namespace SirEngine
