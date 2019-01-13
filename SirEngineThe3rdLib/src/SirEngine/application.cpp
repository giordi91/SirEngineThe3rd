#include "SirEngine/application.h"
#include "SirEngine/layer.h"
#include "SirEngine/log.h"

namespace SirEngine {

Application::Application() {

  m_window = Window::create();
  m_window->setEventCallback([this](Event &e) -> void { this->onEvent(e); });
}

Application::~Application() {

  if (m_window != nullptr) {
    delete m_window;
  }
}
void Application::run() {
  while (m_run) {
    m_window->OnUpdate();
    for (Layer *l : m_layerStack) {
      l->onUpdate();
    }
  }
}
void Application::onEvent(Event &e) {
  SE_CORE_INFO("{0}", e);
  // close event dispatch
  EventDispatcher dispatcher(e);
  dispatcher.dispatch<WindowCloseEvent>(
      [this](WindowCloseEvent &e) -> bool { return (this->onCloseWindow(e)); });

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
void Application::pushLayer(Layer *layer) { m_layerStack.pushLayer(layer); }
void Application::pushOverlay(Layer *layer) {
  m_layerStack.pushOverlayLayer(layer);
}
} // namespace SirEngine
