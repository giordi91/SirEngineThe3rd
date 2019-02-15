#include "SirEngine/application.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/graphicsCore.h"
#include "SirEngine/layer.h"
#include "SirEngine/log.h"
#include "layers/graphics3DLayer.h"
#include "layers/imguiLayer.h"
#include <random>

namespace SirEngine {

Application::Application() {

  m_window = Window::create();
  m_window->setEventCallback([this](Event &e) -> void { this->onEvent(e); });

  imGuiLayer = new ImguiLayer();
  graphicsLayer = new Graphics3DLayer();
  m_layerStack.pushLayer(graphicsLayer);
  // m_layerStack.pushLayer(imGuiLayer);
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

    std::mt19937_64 eng{std::random_device{}()}; // or seed however you want
    std::uniform_int_distribution<> dist{0, 20};
    //std::this_thread::sleep_for(std::chrono::milliseconds{dist(eng)});
  }

  //lets make sure any graphics operation are done
  graphics::stopGraphics();

  // lets clean up the layers, now is safe to free up resources
  for (Layer *l : m_layerStack) {
    l->clear();
  }

  //shutdown anything graphics related;
  graphics::shutdownGraphics();
}
void Application::onEvent(Event &e) {
  // close event dispatch
	SE_CORE_INFO("{0}",e);
  EventDispatcher dispatcher(e);
  dispatcher.dispatch<WindowCloseEvent>(
      [this](WindowCloseEvent &e) -> bool { return (this->onCloseWindow(e)); });
  if (e.handled()) {
    return;
  }
  dispatcher.dispatch<WindowResizeEvent>([this](WindowResizeEvent &e) -> bool {
    return (this->onResizeWindow(e));
  });

  for (auto it = m_layerStack.end(); it != m_layerStack.begin();) {
    (*--it)->onEvent(e);
    if (e.handled()) {
      break;
    }
  }
}
bool Application::onCloseWindow(WindowCloseEvent &e) {
  // graphics::shutdown();
  m_run = false;
  return true;
}
bool Application::onResizeWindow(WindowResizeEvent &e) {
  globals::SCREEN_WIDTH = e.getWidth();
  globals::SCREEN_HEIGHT = e.getHeight();

  m_window->onResize(globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT);
  graphics::onResize(globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT);
  return true;
}
void Application::pushLayer(Layer *layer) { m_layerStack.pushLayer(layer); }
void Application::pushOverlay(Layer *layer) {
  m_layerStack.pushOverlayLayer(layer);
}
} // namespace SirEngine
