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
  void queueEventForEndOfFrame(Event *e) const;
  void pushLayer(Layer *layer);

private:
  bool onCloseWindow(WindowCloseEvent &e);
  bool onResizeWindow(WindowResizeEvent &e);
  inline void flipEndOfFrameQueue() {
    m_queueEndOfFrameCounter = (m_queueEndOfFrameCounter + 1) % 2;
    m_queuedEndOfFrameEventsCurrent =
        &m_queuedEndOfFrameEvents[m_queueEndOfFrameCounter];
  };

private:
  struct EventQueue {
    Event**events = nullptr;
    uint32_t totalSize = 0;
    uint32_t allocCount = 0;
  };

private:
  EventQueue m_queuedEndOfFrameEvents[2];
  EventQueue *m_queuedEndOfFrameEventsCurrent;
  uint32_t m_queueEndOfFrameCounter = 0;
  BaseWindow *m_window = nullptr;
  bool m_run = true;
  LayerStack m_layerStack;
  Layer *imGuiLayer;
  Layer *graphicsLayer;
  static constexpr uint32_t RESERVE_ALLOC_EVENT_QUEUE = 200;
};

// To be implemented by the client
Application *createApplication();
} // namespace SirEngine
