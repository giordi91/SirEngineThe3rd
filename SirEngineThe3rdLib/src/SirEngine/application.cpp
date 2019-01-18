#include "SirEngine/application.h"
#include "SirEngine/graphics/graphicsCore.h"
#include "SirEngine/layer.h"
#include "SirEngine/log.h"
#include "layers/imguiLayer.h"
#include "SirEngine/globals.h"

namespace SirEngine {

Application::Application() {

  m_window = Window::create();
  m_window->setEventCallback([this](Event &e) -> void { this->onEvent(e); });

  layer = new ImguiLayer();
  m_layerStack.pushLayer(layer);
}

Application::~Application() {

  if (m_window != nullptr) {
    delete m_window;
  }
}
void Application::run() {
  while (m_run) {
    m_window->onUpdate();
    graphics::newFrame();
    for (Layer *l : m_layerStack) {
      l->onUpdate();
    }
    graphics::dispatchFrame();
    SE_CORE_INFO("FRAME");
  }
}
void Application::onEvent(Event &e) {
  SE_CORE_INFO("{0}", e);
  // close event dispatch
  EventDispatcher dispatcher(e);
  dispatcher.dispatch<WindowCloseEvent>(
      [this](WindowCloseEvent &e) -> bool { return (this->onCloseWindow(e)); });
  if (e.handled()) {
    return;
  }
  dispatcher.dispatch<WindowResizeEvent>(
      [this](WindowResizeEvent &e) -> bool { return (this->onResizeWindow(e)); });

  for (auto it = m_layerStack.end(); it != m_layerStack.begin();) {
    (*--it)->onEvent(e);
    if (e.handled()) {
      break;
    }
  }
}
bool Application::onCloseWindow(WindowCloseEvent &e) {
  m_run = false;
  return true;
}
bool Application::onResizeWindow(WindowResizeEvent & e)
{
	Globals::SCREEN_WIDTH = e.getWidth();
	Globals::SCREEN_HEIGHT= e.getHeight();

	m_window->onResize(Globals::SCREEN_WIDTH, Globals::SCREEN_HEIGHT);
	graphics::onResize(Globals::SCREEN_WIDTH, Globals::SCREEN_HEIGHT);
	return true;
}
void Application::pushLayer(Layer *layer) { m_layerStack.pushLayer(layer); }
void Application::pushOverlay(Layer *layer) {
  m_layerStack.pushOverlayLayer(layer);
}
} // namespace SirEngine
