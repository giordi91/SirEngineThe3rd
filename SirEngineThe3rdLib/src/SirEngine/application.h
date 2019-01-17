#pragma once
#include "Window.h"
#include "core.h"

#include "SirEngine/events/applicationEvent.h"
#include "SirEngine/events/event.h"
#include "SirEngine/layerStack.h"
namespace SirEngine {
class SIR_ENGINE_API Application {
public:
  Application();
  virtual ~Application();
  void run();

  void onEvent(Event &e);
  bool onCloseWindow(WindowCloseEvent &e);
  void pushLayer(Layer *layer);
  void pushOverlay(Layer *layer);

private:
  Window *m_window = nullptr;
  bool m_run = true;
  LayerStack m_layerStack;
  Layer* layer;
};

// To be implemented by the client
Application *createApplication();
} // namespace SirEngine
