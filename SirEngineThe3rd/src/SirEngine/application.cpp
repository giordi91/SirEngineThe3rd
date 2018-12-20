#include "SirEnginepch.h"

#include "SirEngine/application.h"
#include "SirEngine/log.h"

namespace SirEngine {

Application::Application() {

  m_window = std::unique_ptr<Window>(Window::create());
  m_window->setEventCallback(
      std::bind(&Application::onEvent, this, std::placeholders::_1));
}

Application::~Application() {}
void Application::run() {
  while (m_run) {
    m_window->OnUpdate();
  }
}
void Application::onEvent(Event &e) {
  SE_CORE_INFO("{0}", e);
  EventDispatcher dispatcher(e);
  dispatcher.dispatch<WindowCloseEvent>(
      std::bind(&Application::onCloseWindow, this, std::placeholders::_1));
}
bool Application::onCloseWindow(WindowCloseEvent &e) {
  m_run = false;
  return true;
}
} // namespace SirEngine
