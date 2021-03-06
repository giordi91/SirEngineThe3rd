#include "SirEngine/layers/graphicsLayer.h"

#include "SirEngine/application.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/constantBufferManager.h"
#include "SirEngine/events/keyboardEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/events/renderGraphEvent.h"
#include "SirEngine/events/shaderCompileEvent.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"
#include "SirEngine/graphics/commandBufferManager.h"
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
#include "SirEngine/io/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/textureManager.h"

namespace SirEngine {
void GraphicsLayer::allocateOffscreenBuffer(uint32_t w, uint32_t h) {
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::VULKAN) {
    offscreenBuffer = globals::TEXTURE_MANAGER->allocateTexture(
        w, h, RenderTargetFormat::BGRA32, "offscreenBuffer",
        TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::RENDER_TARGET |
            TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::SHADER_RESOURCE,
        RESOURCE_STATE::SHADER_READ_RESOURCE);
  } else {
    offscreenBuffer = globals::TEXTURE_MANAGER->allocateTexture(
        w, h, RenderTargetFormat::RGBA32, "offscreenBuffer",
        TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::RENDER_TARGET |
            TextureManager::TEXTURE_ALLOCATION_FLAG_BITS::SHADER_RESOURCE,
        RESOURCE_STATE::SHADER_READ_RESOURCE);
  }
  globals::OFFSCREEN_BUFFER = offscreenBuffer;
}

void GraphicsLayer::prepareRenderGraph()
{
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

	globals::RENDERING_GRAPH->finalizeGraph(m_workerBuffer, &m_graphContext);
}

void GraphicsLayer::onAttach() {
  m_workerBuffer = globals::COMMAND_BUFFER_MANAGER->createBuffer(
      CommandBufferManager::COMMAND_BUFFER_ALLOCATION_NONE, "resize");
  globals::COMMAND_BUFFER_MANAGER->resetBufferHandle(m_workerBuffer);

  globals::MAIN_CAMERA = new Camera3DPivot();
  globals::DEBUG_CAMERA = new Camera3DPivot();
  globals::ACTIVE_CAMERA = globals::MAIN_CAMERA;

  // camera in dx12 has negate panX and rotateX, unsure why, might be because
  // vulkan has the negative viewport?
  float negate =
      globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12 ? -1.0f : 1.0f;
  CameraManipulationConfig camConfig{
      -0.01f * negate, 0.01f, 0.012f * negate, 0.012f, -0.07f,
  };

  // uint32_t w = globals::ENGINE_CONFIG->m_windowWidth;
  // uint32_t h = globals::ENGINE_CONFIG->m_windowHeight;
  uint32_t w = 1620;
  uint32_t h = 924;
  allocateOffscreenBuffer(w, h);

  // render graph context
  m_graphContext = {offscreenBuffer, w, h};

  globals::MAIN_CAMERA->setManipulationMultipliers(camConfig);
  globals::MAIN_CAMERA->setCameraPhyisicalParameters(60.0f, 0.1, 200.0f);
  globals::MAIN_CAMERA->setLookAt(0, 15, 0);
  globals::MAIN_CAMERA->setPosition(0, 15, 15);
  globals::MAIN_CAMERA->updateCamera(w, h,true);

  globals::DEBUG_CAMERA->setManipulationMultipliers(camConfig);
  globals::DEBUG_CAMERA->setCameraPhyisicalParameters(60.0f, 0.1, 200.0f);
  globals::DEBUG_CAMERA->setLookAt(0, 15, 0);
  globals::DEBUG_CAMERA->setPosition(0, 15, 15);
  globals::DEBUG_CAMERA->updateCamera(w, h,true);

  globals::RENDERING_CONTEXT->executeGlobalCommandList();
  globals::RENDERING_CONTEXT->flush();
  globals::RENDERING_CONTEXT->resetGlobalCommandList();
  // globals::ASSET_MANAGER->loadScene(globals::ENGINE_CONFIG->m_startScenePath);
  globals::ASSET_MANAGER->loadScene("../data/scenes/tempScene.json");

  prepareRenderGraph();
  globals::RENDERING_CONTEXT->executeGlobalCommandList();
  globals::RENDERING_CONTEXT->flush();
}

void GraphicsLayer::onDetach() {}
void GraphicsLayer::onUpdate() {
  uint32_t w = globals::ENGINE_CONFIG->m_windowWidth;
  uint32_t h = globals::ENGINE_CONFIG->m_windowHeight;
  if (m_graphContext.m_renderTarget.isHandleValid()) {
    w = m_graphContext.renderTargetWidth;
    h = m_graphContext.renderTargetHeight;
  }
  bool updateCamera = isFlagSet(globals::ENGINE_FLAGS->m_interaction,
            EngineFlagsInteraction::EngineFlagsInteraction_AllowInput);
      globals::RENDERING_CONTEXT->setupCameraForFrame(w, h,updateCamera);
  // evaluating rendering graph
  globals::CONSTANT_BUFFER_MANAGER->processBufferedData();
  globals::DEBUG_RENDERER->newFrame();

  if (globals::ACTIVE_CAMERA == globals::DEBUG_CAMERA) {
    globals::DEBUG_RENDERER->drawCamera(globals::MAIN_CAMERA, w,h,
                                        glm::vec4(1, 1, 1, 1));
  }

  globals::RENDERING_GRAPH->compute(&m_graphContext);
}

#define SE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
void GraphicsLayer::onEvent(Event &event) {
  EventDispatcher dispatcher(event);
  dispatcher.dispatch<MouseButtonPressEvent>(
      SE_BIND_EVENT_FN(GraphicsLayer::onMouseButtonPressEvent));
  dispatcher.dispatch<MouseButtonReleaseEvent>(
      SE_BIND_EVENT_FN(GraphicsLayer::onMouseButtonReleaseEvent));
  dispatcher.dispatch<MouseMoveEvent>(
      SE_BIND_EVENT_FN(GraphicsLayer::onMouseMoveEvent));
  dispatcher.dispatch<KeyboardReleaseEvent>(
      SE_BIND_EVENT_FN(GraphicsLayer::onKeyboardReleaseEvent));
  dispatcher.dispatch<WindowResizeEvent>(
      SE_BIND_EVENT_FN(GraphicsLayer::onResizeEvent));
  dispatcher.dispatch<ShaderCompileEvent>(
      SE_BIND_EVENT_FN(GraphicsLayer::onShaderCompileEvent));
  dispatcher.dispatch<RenderSizeChanged>(
      SE_BIND_EVENT_FN(GraphicsLayer::onRenderSizeChanged));
  // dispatcher.dispatch<ReloadScriptsEvent>(
  //    SE_BIND_EVENT_FN(VkTempLayer::onReloadScriptEvent));
}
#undef SE_BIND_EVENT_FN

void GraphicsLayer::clear() {
  globals::COMMAND_BUFFER_MANAGER->freeBuffer(m_workerBuffer);
  globals::TEXTURE_MANAGER->free(offscreenBuffer);
  globals::RENDERING_GRAPH->clear();
}

bool GraphicsLayer::onMouseButtonPressEvent(MouseButtonPressEvent &e) {
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

bool GraphicsLayer::onMouseButtonReleaseEvent(MouseButtonReleaseEvent &e) {
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

bool GraphicsLayer::onMouseMoveEvent(MouseMoveEvent &) { return true; }

bool GraphicsLayer::onKeyboardReleaseEvent(KeyboardReleaseEvent &e) {
  // TODO use keycodes
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

bool GraphicsLayer::onResizeEvent(WindowResizeEvent &e) {
  // First we flush to make sure there are no inflight frames
  globals::RENDERING_CONTEXT->flush();

  // next we can issue a resize
  const uint32_t w = e.getWidth();
  const uint32_t h = e.getHeight();
  m_graphContext.renderTargetWidth = w;
  m_graphContext.renderTargetHeight = h;
  globals::RENDERING_GRAPH->onResizeEvent(w, h, m_workerBuffer,
                                          &m_graphContext);

  // now we flush and reset
  globals::COMMAND_BUFFER_MANAGER->executeFlushAndReset(m_workerBuffer);
  // globals::RENDERING_CONTEXT->executeGlobalCommandList();
  // globals::RENDERING_CONTEXT->flush();
  // globals::RENDERING_CONTEXT->resetGlobalCommandList();

  return false;
}

bool GraphicsLayer::onShaderCompileEvent(ShaderCompileEvent &e) {
  SE_CORE_INFO("Reading to compile shader");
  globals::PSO_MANAGER->recompilePSOFromShader(e.getShader(),
                                               e.getOffsetPath());
  return true;
}

bool GraphicsLayer::onRenderSizeChanged(RenderSizeChanged &e) {
  // First we flush to make sure there are no inflight frames
  globals::RENDERING_CONTEXT->flush();

  // next we can issue a resize
  const uint32_t w = e.getWidth();
  const uint32_t h = e.getHeight();

  globals::TEXTURE_MANAGER->free(offscreenBuffer);
  allocateOffscreenBuffer(w, h);
  m_graphContext = {offscreenBuffer, w, h};

  globals::RENDERING_GRAPH->onResizeEvent(w, h, m_workerBuffer,
                                          &m_graphContext);
  SE_CORE_INFO("{}", e.toString());

  // now we flush and reset
  globals::COMMAND_BUFFER_MANAGER->executeFlushAndReset(m_workerBuffer);
  return true;
}

/*
bool GraphicsLayer::onReloadScriptEvent(ReloadScriptsEvent &) {
  globals::SCRIPTING_CONTEXT->reloadContext();
  return true;
}*/
}  // namespace SirEngine
