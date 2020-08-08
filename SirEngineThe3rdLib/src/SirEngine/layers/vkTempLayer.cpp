#include "SirEngine/layers/vkTempLayer.h"

#include "SirEngine/application.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/bufferManager.h"
#include "SirEngine/constantBufferManager.h"
#include "SirEngine/events/debugEvent.h"
#include "SirEngine/events/keyboardEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/events/shaderCompileEvent.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"
#include "SirEngine/graphics/debugRenderer.h"
#include "SirEngine/graphics/nodes/FinalBlitNode.h"
#include "SirEngine/graphics/nodes/debugDrawNode.h"
#include "SirEngine/graphics/nodes/forwardPlus.h"
#include "SirEngine/graphics/nodes/skybox.h"
#include "SirEngine/graphics/postProcess/effects/gammaAndToneMappingEffect.h"
#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/input.h"
#include "SirEngine/interopData.h"
#include "SirEngine/log.h"
#include "SirEngine/psoManager.h"

namespace SirEngine {

void VkTempLayer::initGrass() {}

void VkTempLayer::onAttach() {
  globals::MAIN_CAMERA = new Camera3DPivot();
  globals::DEBUG_CAMERA = new Camera3DPivot();
  globals::ACTIVE_CAMERA = globals::MAIN_CAMERA;
  // globals::MAIN_CAMERA->setLookAt(0, 125, 0);
  // globals::MAIN_CAMERA->setPosition(00, 125, 60);
  // camera in dx12 has negate panX and rotateX, unsure why, might be because
  // vulkan has the negative viewport?
  float negate =
      globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12 ? -1.0f : 1.0f;
  CameraManipulationConfig camConfig{
      -0.01f * negate, 0.01f, 0.012f * negate, 0.012f, -0.07f,
  };
  globals::MAIN_CAMERA->setManipulationMultipliers(camConfig);
  globals::MAIN_CAMERA->setCameraPhyisicalParameters(60.0f, 0.01, 1000.0f);
  globals::MAIN_CAMERA->setLookAt(0, 15, 0);
  globals::MAIN_CAMERA->setPosition(0, 15, 15);
  globals::MAIN_CAMERA->updateCamera();

  globals::DEBUG_CAMERA->setManipulationMultipliers(camConfig);
  globals::DEBUG_CAMERA->setCameraPhyisicalParameters(60.0f, 0.01, 1000.0f);
  globals::DEBUG_CAMERA->setLookAt(0, 15, 0);
  globals::DEBUG_CAMERA->setPosition(0, 15, 15);
  globals::DEBUG_CAMERA->updateCamera();

  globals::RENDERING_CONTEXT->flush();
  globals::RENDERING_CONTEXT->resetGlobalCommandList();
  // globals::ASSET_MANAGER->loadScene(globals::ENGINE_CONFIG->m_startScenePath);
  globals::ASSET_MANAGER->loadScene("../data/scenes/tempScene.json");

  initGrass();

  alloc =
      new GraphAllocators{globals::STRING_POOL, globals::PERSISTENT_ALLOCATOR};

  globals::RENDERING_GRAPH = new DependencyGraph();
  auto *const forward = new ForwardPlus(*alloc);
  auto *const skybox = new SkyBoxPass(*alloc);
  auto *const debugDraw = new DebugDrawNode(*alloc);
  auto *const finalBlit = new FinalBlitNode(*alloc);
  auto *postProcess = new PostProcessStack(*alloc);
  postProcess->allocateRenderPass<GammaAndToneMappingEffect>(
      "GammaToneMapping");
  postProcess->initialize();

  // add callback to forward for grass shader
  forward->addCallbackConfig(graphics::GrassTechnique::GRASS_TECHNIQUE_FORWARD,
                             &m_grass);

  // temporary graph for testing
  globals::RENDERING_GRAPH->addNode(forward);
  globals::RENDERING_GRAPH->addNode(skybox);
  globals::RENDERING_GRAPH->addNode(debugDraw);
  globals::RENDERING_GRAPH->addNode(finalBlit);
  globals::RENDERING_GRAPH->addNode(postProcess);
  globals::RENDERING_GRAPH->setFinalNode(finalBlit);

  SirEngine::DependencyGraph::connectNodes(forward, ForwardPlus::OUT_TEXTURE,
                                           skybox, SkyBoxPass::IN_TEXTURE);
  SirEngine::DependencyGraph::connectNodes(forward, ForwardPlus::DEPTH_RT,
                                           skybox, SkyBoxPass::DEPTH);

  SirEngine::DependencyGraph::connectNodes(
      skybox, SkyBoxPass::OUT_TEX, debugDraw, DebugDrawNode::IN_TEXTURE);
  SirEngine::DependencyGraph::connectNodes(forward, ForwardPlus::DEPTH_RT,
                                           debugDraw, DebugDrawNode::DEPTH_RT);

  globals::RENDERING_GRAPH->connectNodes(debugDraw, DebugDrawNode::OUT_TEXTURE,
                                         postProcess,
                                         PostProcessStack::IN_TEXTURE);
  globals::RENDERING_GRAPH->connectNodes(
      forward, ForwardPlus::DEPTH_RT, postProcess, PostProcessStack::DEPTH_RT);
  SirEngine::DependencyGraph::connectNodes(
      postProcess, PostProcessStack::OUT_TEXTURE, finalBlit,
      FinalBlitNode::IN_TEXTURE);

  // TODO this whole reset execute flush needs to be reworked
  globals::RENDERING_GRAPH->finalizeGraph();
  globals::RENDERING_CONTEXT->executeGlobalCommandList();
  globals::RENDERING_CONTEXT->flush();
}

void VkTempLayer::onDetach() {}
void VkTempLayer::onUpdate() {
  globals::RENDERING_CONTEXT->setupCameraForFrame();

  // evaluating rendering graph
  globals::CONSTANT_BUFFER_MANAGER->processBufferedData();
  globals::DEBUG_RENDERER->newFrame();
  globals::RENDERING_GRAPH->compute();
}
void VkTempLayer::onEvent(Event &event) {
  EventDispatcher dispatcher(event);
  dispatcher.dispatch<MouseButtonPressEvent>(
      SE_BIND_EVENT_FN(VkTempLayer::onMouseButtonPressEvent));
  dispatcher.dispatch<MouseButtonReleaseEvent>(
      SE_BIND_EVENT_FN(VkTempLayer::onMouseButtonReleaseEvent));
  dispatcher.dispatch<MouseMoveEvent>(
      SE_BIND_EVENT_FN(VkTempLayer::onMouseMoveEvent));
  dispatcher.dispatch<KeyboardReleaseEvent>(
      SE_BIND_EVENT_FN(VkTempLayer::onKeyboardReleaseEvent));
  dispatcher.dispatch<WindowResizeEvent>(
      SE_BIND_EVENT_FN(VkTempLayer::onResizeEvent));
  dispatcher.dispatch<ShaderCompileEvent>(
      SE_BIND_EVENT_FN(VkTempLayer::onShaderCompileEvent));
  // dispatcher.dispatch<ReloadScriptsEvent>(
  //    SE_BIND_EVENT_FN(VkTempLayer::onReloadScriptEvent));
}

void VkTempLayer::clear() { globals::RENDERING_GRAPH->clear(); }

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
  /*
const float deltaX = previousX - e.getX();
const float deltaY = previousY - e.getY();
if (leftDown) {
  globals::ACTIVE_CAMERA->rotCamera(deltaX, deltaY);
} else if (middleDown) {
  globals::ACTIVE_CAMERA->panCamera(deltaX, deltaY);
} else if (rightDown) {
  globals::ACTIVE_CAMERA->zoomCamera(deltaX);
}

// storing old position
previousX = e.getX();
previousY = e.getY();
*/
  return true;
}

bool VkTempLayer::onKeyboardReleaseEvent(KeyboardReleaseEvent &e) {
  // TODO use keycodes
  int cCode = 67;
  int shiftCode = 16;
  bool isShiftDown = globals::INPUT->isKeyDown(shiftCode);
  if (isShiftDown & (e.getKeyCode() == 67)) {
    SE_CORE_INFO("swapping driving camera {}", e.getKeyCode());
    globals::ACTIVE_CAMERA = globals::ACTIVE_CAMERA == globals::MAIN_CAMERA
                                 ? globals::DEBUG_CAMERA
                                 : globals::MAIN_CAMERA;
    return true;
  }

  return false;
}

bool VkTempLayer::onResizeEvent(WindowResizeEvent &e) {
  // need to recreate the frame buffer, this is temporary
  // TODO re-add resizing
  // createRenderTargetAndFrameBuffer(globals::ENGINE_CONFIG->m_windowWidth,
  //                                 globals::ENGINE_CONFIG->m_windowHeight);

  return true;
}

bool VkTempLayer::onShaderCompileEvent(ShaderCompileEvent &e) {
  SE_CORE_INFO("Reading to compile shader");
  globals::PSO_MANAGER->recompilePSOFromShader(e.getShader(),
                                               e.getOffsetPath());
  return true;
}

/*
bool VkTempLayer::onReloadScriptEvent(ReloadScriptsEvent &) {
  globals::SCRIPTING_CONTEXT->reloadContext();
  return true;
}*/
}  // namespace SirEngine
