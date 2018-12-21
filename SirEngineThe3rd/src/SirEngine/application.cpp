#include "SirEnginepch.h"

#include "SirEngine/application.h"
#include "SirEngine/log.h"

namespace SirEngine {

Application::Application() {

  m_window = Window::create();
  m_window->setEventCallback(
      [this](Event &e) -> void { this->onEvent(e); });
}

Application::~Application() {

  if (m_window != nullptr) {
    delete m_window;
  }
}
void Application::run() {
  while (m_run) {
    m_window->OnUpdate();
  }
}
void Application::onEvent(Event &e) {
  SE_CORE_INFO("{0}", e);
  // close event dispatch
  EventDispatcher dispatcher(e);
  dispatcher.dispatch<WindowCloseEvent>(
      [this](WindowCloseEvent &e) -> bool { return (this->onCloseWindow(e)); });
}
bool Application::onCloseWindow(WindowCloseEvent &e) {
  m_run = false;
  return true;
}
} // namespace SirEngine
