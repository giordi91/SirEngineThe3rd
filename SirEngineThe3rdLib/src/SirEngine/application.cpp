#include "SirEngine/application.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/graphicsCore.h"
#include "SirEngine/layer.h"
#include "SirEngine/log.h"
#include "engineConfig.h"
#include "fileUtils.h"
#include "layers/graphics3DLayer.h"
#include "layers/imguiLayer.h"
#include <random>

#include "SirEngine/input.h"
#include "SirEngine/runtimeString.h"

namespace SirEngine {

static const char *CONFIG_PATH = "../data/engineConfig.json";

void Application::loadConfigFile() {
  // try to read the configuration file
  bool exists = fileExists(CONFIG_PATH);
  globals::ENGINE_CONFIG = reinterpret_cast<EngineConfig *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(EngineConfig)));
  if (exists) {
    parseConfigFile(CONFIG_PATH, *globals::ENGINE_CONFIG);
  } else {
    initializeConfigDefault(*globals::ENGINE_CONFIG);
  }
}

Application::Application() {
  // initializing allocators as first thing so everything else can use it
  globals::STRING_POOL = new StringPool(2 << 22); // 4 megabyte allocation
  globals::FRAME_ALLOCATOR = new StackAllocator();
  // TODO fix the interface to be same as other allocators
  globals::FRAME_ALLOCATOR->initialize(2 << 22);
  // allocating 20 mb
  // TODO this should be a settings from somewhere
  globals::PERSISTENT_ALLOCATOR = new ThreeSizesPool(20 * 1024 * 1024);

  loadConfigFile();

  m_window = BaseWindow::create();
  m_window->setEventCallback([this](Event &e) -> void { this->onEvent(e); });

  m_queuedEndOfFrameEvents[0].events =
      static_cast<Event **>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(Event *) * RESERVE_ALLOC_EVENT_QUEUE));
  m_queuedEndOfFrameEvents[0].totalSize = RESERVE_ALLOC_EVENT_QUEUE;
  m_queuedEndOfFrameEvents[1].events =
      static_cast<Event **>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(Event *) * RESERVE_ALLOC_EVENT_QUEUE));
  m_queuedEndOfFrameEvents[1].totalSize = RESERVE_ALLOC_EVENT_QUEUE;
  m_queuedEndOfFrameEventsCurrent = &m_queuedEndOfFrameEvents[0];

  imGuiLayer = new ImguiLayer();
  graphicsLayer = new Graphics3DLayer();
  m_layerStack.pushLayer(graphicsLayer);
  m_layerStack.pushLayer(imGuiLayer);
  globals::APPLICATION = this;
}

Application::~Application() { delete m_window; }
void Application::run() {
  while (m_run) {
    globals::LAST_FRAME_TIME_NS = globals::GAME_CLOCK.getDelta();
    ++globals::TOTAL_NUMBER_OF_FRAMES;
    m_window->onUpdate();
    graphics::newFrame();

    const int count = m_layerStack.count();
    Layer **layers = m_layerStack.begin();
    for (int i = 0; i < count; ++i) {
      layers[i]->onUpdate();
    }
    graphics::dispatchFrame();
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
  graphics::stopGraphics();

  // lets clean up the layers, now is safe to free up resources
  const int count = m_layerStack.count();
  Layer **layers = m_layerStack.begin();
  for (int i = 0; i < count; ++i) {
    layers[i]->clear();
  }

  // shutdown anything graphics related;
  graphics::shutdownGraphics();
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
  globals::SCREEN_WIDTH = e.getWidth();
  globals::SCREEN_HEIGHT = e.getHeight();

  m_window->onResize(globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT);
  graphics::onResize(globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT);

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
