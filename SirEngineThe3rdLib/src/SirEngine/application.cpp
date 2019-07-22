#include "SirEngine/application.h"
#include <random>
#include "SirEngine/globals.h"
#include "SirEngine/graphics/graphicsCore.h"
#include "SirEngine/layer.h"
#include "SirEngine/log.h"
#include "fileUtils.h"
#include "layers/graphics3DLayer.h"
#include "layers/imguiLayer.h"

#include "SirEngine/runtimeString.h"

namespace SirEngine {

static const std::string CONFIG_PATH = "engineConfig.json";
static const std::string CONFIG_DATA_SOURCE_KEY = "dataSource";
static const std::string CONFIG_STARTING_SCENE_KEY = "startingScene";
static const std::string DEFAULT_STRING = "";

void Application::parseConfigFile() {
  // try to read the configuration file
  nlohmann::json jobj = getJsonObj(CONFIG_PATH);
  globals::DATA_SOURCE_PATH = persistentString(
      getValueIfInJson(jobj, CONFIG_DATA_SOURCE_KEY, DEFAULT_STRING).c_str());
  assert(globals::DATA_SOURCE_PATH[0]!= '\0');
  globals::START_SCENE_PATH = persistentString(
      getValueIfInJson(jobj, CONFIG_STARTING_SCENE_KEY, DEFAULT_STRING)
          .c_str());
  assert(globals::START_SCENE_PATH[0] != '\0');
}

Application::Application() {
  // initializing allocators as first thing so everything else can use it 
  globals::STRING_POOL = new StringPool(2 << 22);  // 4 megabyte allocation
  globals::FRAME_ALLOCATOR = new StackAllocator();
  globals::FRAME_ALLOCATOR->initialize(2 << 22);

  parseConfigFile();

  m_window = Window::create();
  m_window->setEventCallback([this](Event &e) -> void { this->onEvent(e); });
  m_queuedEndOfFrameEvents.resize(2);
  m_queuedEndOfFrameEvents[0].reserve(10);
  m_queuedEndOfFrameEvents[1].reserve(10);
  m_queuedEndOfFrameEventsCurrent = &m_queuedEndOfFrameEvents[0];

  imGuiLayer = new ImguiLayer();
  graphicsLayer = new Graphics3DLayer();
  m_layerStack.pushLayer(graphicsLayer);
  m_layerStack.pushOverlayLayer(imGuiLayer);
  globals::APPLICATION = this;
}

Application::~Application() { delete m_window; }
void Application::run() {
  while (m_run) {
    globals::LAST_FRAME_TIME_NS = globals::GAME_CLOCK.getDelta();
    ++globals::TOTAL_NUMBER_OF_FRAMES;
    m_window->onUpdate();
    graphics::newFrame();

    for (Layer *l : m_layerStack) {
      l->onUpdate();
    }
    graphics::dispatchFrame();

    auto currentQueue = m_queuedEndOfFrameEventsCurrent;
    flipEndOfFrameQueue();
    for (auto e : (*currentQueue)) {
      onEvent(*e);
      delete e;
    }
    currentQueue->clear();
  }

  // lets make sure any graphics operation are done
  graphics::stopGraphics();

  // lets clean up the layers, now is safe to free up resources
  for (Layer *l : m_layerStack) {
    l->clear();
  }

  // shutdown anything graphics related;
  graphics::shutdownGraphics();
}
void Application::queueEventForEndOfFrame(Event *e) {
  m_queuedEndOfFrameEventsCurrent->push_back(e);
}
void Application::onEvent(Event &e) {
  // close event dispatch
  SE_CORE_INFO("{0}", e);
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
  for (auto it = m_layerStack.end(); it != m_layerStack.begin();) {
    (*--it)->onEvent(e);
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
  for (auto it = m_layerStack.end(); it != m_layerStack.begin();) {
    (*--it)->onEvent(e);
    if (e.handled()) {
      break;
    }
  }
  return true;
}

void Application::pushLayer(Layer *layer) { m_layerStack.pushLayer(layer); }
void Application::pushOverlay(Layer *layer) {
  m_layerStack.pushOverlayLayer(layer);
}
}  // namespace SirEngine
