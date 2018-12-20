#include "SirEnginepch.h"
#include "application.h"

namespace SirEngine {

Application::Application() {
	m_window = std::unique_ptr<Window>(Window::create());
}

Application::~Application() {}
void Application::run() {
  while (true) {
	  m_window->OnUpdate();
  }

}
} // namespace SirEngine
