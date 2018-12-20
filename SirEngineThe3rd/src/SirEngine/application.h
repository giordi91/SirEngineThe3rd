#pragma once
#include "Window.h"
#include "core.h"

#include "SirEngine/events/event.h"
#include "SirEngine/events/appliacationEvent.h"
namespace SirEngine {
class SIR_ENGINE_API Application {
public:
  Application();
  virtual ~Application();
  void run();

  void onEvent(Event &e);
  bool onCloseWindow(WindowCloseEvent& e);

private:
  std::unique_ptr<Window> m_window;
  bool m_run = true;
};

// To be implemented by the client
Application *createApplication();
} // namespace SirEngine
