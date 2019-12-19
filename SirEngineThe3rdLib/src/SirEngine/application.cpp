#include "SirEngine/application.h"
#include "SirEngine/globals.h"
#include "SirEngine/layer.h"
#include "SirEngine/log.h"
#include "engineConfig.h"
#include "fileUtils.h"
#include "layers/graphics3DLayer.h"
#include "layers/imguiLayer.h"
#include <random>

#include "SirEngine/input.h"
#include "graphics/renderingContext.h"
#include "layers/vkTempLayer.h"

namespace SirEngine {

static const char *CONFIG_PATH = "../data/engineConfig.json";

Application::Application() {

  // this is in charge to start up the engine basic systems

  EngineInitializationConfig engineConfig{};
  engineConfig.configPath = CONFIG_PATH;
  engineConfig.initCoreWithNoConfig = false;
  initializeEngine(engineConfig);

  WindowProps windowProperty{};
  windowProperty.width = globals::ENGINE_CONFIG->m_windowWidth;
  windowProperty.height = globals::ENGINE_CONFIG->m_windowHeight;
  windowProperty.title = globals::ENGINE_CONFIG->m_windowTitle;

  m_window = BaseWindow::create(windowProperty);
  m_window->setEventCallback([this](Event &e) -> void { this->onEvent(e); });

  // now that the window is created we can crate a rendering context
  RenderingContextCreationSettings creationSettings{};
  creationSettings.width = windowProperty.width;
  creationSettings.height = windowProperty.height;
  creationSettings.graphicsAPI = globals::ENGINE_CONFIG->m_graphicsAPI;
  // creationSettings.graphicsAPI = GRAPHIC_API::VULKAN;

  // creationSettings.graphicsAPI = GRAPHIC_API::DX12;
  creationSettings.window = m_window;
  creationSettings.apiConfig = {};

  globals::RENDERING_CONTEXT = RenderingContext::create(
      creationSettings, creationSettings.width, creationSettings.height);
  const bool result = globals::RENDERING_CONTEXT->initializeGraphics();
  if (!result) {
    exit(EXIT_FAILURE);
  }

  m_queuedEndOfFrameEvents[0].events =
      static_cast<Event **>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(Event *) * RESERVE_ALLOC_EVENT_QUEUE));
  m_queuedEndOfFrameEvents[0].totalSize = RESERVE_ALLOC_EVENT_QUEUE;
  m_queuedEndOfFrameEvents[1].events =
      static_cast<Event **>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(Event *) * RESERVE_ALLOC_EVENT_QUEUE));
  m_queuedEndOfFrameEvents[1].totalSize = RESERVE_ALLOC_EVENT_QUEUE;
  m_queuedEndOfFrameEventsCurrent = &m_queuedEndOfFrameEvents[0];

  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
    imGuiLayer = new ImguiLayer();
    graphicsLayer = new Graphics3DLayer();
    m_layerStack.pushLayer(graphicsLayer);
    m_layerStack.pushLayer(imGuiLayer);
  } else {
    graphicsLayer = new VkTempLayer();
    m_layerStack.pushLayer(graphicsLayer);
    //imGuiLayer = new ImguiLayer();
    //m_layerStack.pushLayer(imGuiLayer);
  }
  globals::APPLICATION = this;
}

Application::~Application() { delete m_window; }
void Application::run() {
  while (m_run) {
    globals::LAST_FRAME_TIME_NS = globals::GAME_CLOCK.getDelta();
    ++globals::TOTAL_NUMBER_OF_FRAMES;
    m_window->onUpdate();
    globals::RENDERING_CONTEXT->newFrame();

    const int count = m_layerStack.count();
    Layer **layers = m_layerStack.begin();
    for (int i = 0; i < count; ++i) {
      layers[i]->onUpdate();
    }
    globals::RENDERING_CONTEXT->dispatchFrame();
    // update input to cache current input for next frame
    globals::INPUT->swapFrameKey();

    const auto currentQueue = m_queuedEndOfFrameEventsCurrent;
    flipEndOfFrameQueue();
    for (uint32_t i = 0; i < currentQueue->allocCount; ++i) {
      onEvent(*(currentQueue->events[i]));
      delete currentQueue->events[i];
    }
    currentQueue->allocCount = 0;
  }

  // lets make sure any graphics operation are done
  globals::RENDERING_CONTEXT->stopGraphic();

  // lets clean up the layers, now is safe to free up resources
  const int count = m_layerStack.count();
  Layer **layers = m_layerStack.begin();
  for (int i = 0; i < count; ++i) {
    layers[i]->clear();
  }

  // shutdown anything graphics related;
  globals::RENDERING_CONTEXT->shutdownGraphic();
}
void Application::queueEventForEndOfFrame(Event *e) const {
  const int alloc = m_queuedEndOfFrameEventsCurrent->allocCount;
  m_queuedEndOfFrameEventsCurrent->events[alloc] = e;
  ++(m_queuedEndOfFrameEventsCurrent->allocCount);
}
void Application::onEvent(Event &e) {
  // close event dispatch
  SE_CORE_INFO("{0}", e.toString());
  EventDispatcher dispatcher(e);
  dispatcher.dispatch<WindowCloseEvent>(
      [this](WindowCloseEvent &e) -> bool { return (this->onCloseWindow(e)); });
  if (e.handled()) {
    return;
  }
  dispatcher.dispatch<WindowResizeEvent>([this](WindowResizeEvent &e) -> bool {
    return (this->onResizeWindow(e));
  });

  if (e.handled()) {
    return;
  }

  const int count = m_layerStack.count();
  Layer **layers = m_layerStack.begin();
  for (int i = (count - 1); i >= 0; --i) {
    layers[i]->onEvent(e);
    if (e.handled()) {
      break;
    }
  }
}
bool Application::onCloseWindow(WindowCloseEvent &) {
  // graphics::shutdown();
  m_run = false;
  return true;
}
bool Application::onResizeWindow(WindowResizeEvent &e) {
  globals::ENGINE_CONFIG->m_windowWidth = e.getWidth();
  globals::ENGINE_CONFIG->m_windowHeight = e.getHeight();

  m_window->onResize(globals::ENGINE_CONFIG->m_windowWidth,
                     globals::ENGINE_CONFIG->m_windowHeight);
  globals::RENDERING_CONTEXT->resize(globals::ENGINE_CONFIG->m_windowWidth,
                                     globals::ENGINE_CONFIG->m_windowHeight);

  // push the resize event to everyone in case is needed
  const int count = m_layerStack.count();
  Layer **layers = m_layerStack.begin();
  for (int i = 0; i < count; ++i) {
    layers[i]->onEvent(e);
    if (e.handled()) {
      break;
    }
  }
  return true;
}

void Application::pushLayer(Layer *layer) { m_layerStack.pushLayer(layer); }
} // namespace SirEngine
