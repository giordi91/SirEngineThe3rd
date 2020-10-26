#include "SirEngine/application.h"

#include "SirEngine/engineConfig.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/input.h"
#include "SirEngine/io/fileUtils.h"
#include "SirEngine/layer.h"
#include "SirEngine/layers/imguiDebugLayer.h"
#include "SirEngine/log.h"
#include "flags.h"

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
  windowProperty.startFullScreen = globals::ENGINE_CONFIG->m_startFullScreen;

  // The window might decide to override the size due to fullscreen, if so
  // the value of engine config will change
  m_window = BaseWindow::create(windowProperty);
  m_window->setEventCallback([this](Event &e) -> void { this->onEvent(e); });

  // now that the window is created we can crate a rendering context
  RenderingContextCreationSettings creationSettings{};
  creationSettings.width = globals::ENGINE_CONFIG->m_windowWidth;
  creationSettings.height = globals::ENGINE_CONFIG->m_windowHeight;
  creationSettings.graphicsAPI = globals::ENGINE_CONFIG->m_graphicsAPI;

  creationSettings.window = m_window;
  creationSettings.apiConfig = {};

  globals::ENGINE_FLAGS = new EngineFlags();
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

  globals::APPLICATION = this;
}

Application::~Application() {
  delete globals::ENGINE_FLAGS;
  delete m_window;
}
void Application::run() {
  while (m_run) {
    globals::LAST_FRAME_TIME_NS = globals::GAME_CLOCK.getDelta();
    ++globals::TOTAL_NUMBER_OF_FRAMES;
    m_window->onUpdate();
    globals::RENDERING_CONTEXT->newFrame();
    EventQueue *currentQueue = m_queuedEndOfFrameEventsCurrent;
    flipEndOfFrameQueue();
    for (uint32_t i = 0; i < currentQueue->allocCount; ++i) {
      onEvent(*(currentQueue->events[i]));
      delete currentQueue->events[i];
    }
    currentQueue->allocCount = 0;

    const int count = m_layerStack.count();
    Layer **layers = m_layerStack.begin();
    for (int i = 0; i < count; ++i) {
      layers[i]->onUpdate();
    }
    globals::RENDERING_CONTEXT->dispatchFrame();
    // update input to cache current input for next frame
    globals::INPUT->swapFrameKey();
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
  // SE_CORE_INFO("{0}", e.toString());
  EventDispatcher dispatcher(e);
  dispatcher.dispatch<WindowCloseEvent>(
      [this](WindowCloseEvent &e) -> bool { return (this->onCloseWindow(e)); });
  if (e.handled()) {
    return;
  }
  dispatcher.dispatch<WindowResizeEvent>([this](WindowResizeEvent &e) -> bool {
    return (this->onResizeWindow(e));
  });
  ;
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
}  // namespace SirEngine
