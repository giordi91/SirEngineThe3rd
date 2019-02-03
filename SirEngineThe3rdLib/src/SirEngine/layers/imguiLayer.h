#pragma once

#include "SirEngine/debugUiWidgets/frameTimingsWidget.h"
#include "SirEngine/debugUiWidgets/memoryConsumptionWidget.h"
#include "SirEngine/events/applicationEvent.h"
#include "SirEngine/events/keyboardEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/layer.h"

namespace SirEngine {

class SIR_ENGINE_API ImguiLayer : public Layer {
public:
  ImguiLayer() : Layer("ImGuiLayer") {}
  ~ImguiLayer() override = default;

  void onAttach() override;
  void onDetach() override;
  void onUpdate() override;
  void onEvent(Event &event) override;

private:
  // event implementation for the layer
  bool onMouseButtonPressEvent(const MouseButtonPressEvent &e) const;
  bool onMouseButtonReleaseEvent(const MouseButtonReleaseEvent &e) const;
  bool onMouseMoveEvent(const MouseMoveEvent &e) const;
  bool onMouseScrolledEvent(const MouseScrollEvent &e) const;
  bool onKeyPressedEvent(const KeyboardPressEvent &e);
  bool onKeyReleasedEvent(const KeyboardReleaseEvent &e) const;
  bool onWindowResizeEvent(const WindowResizeEvent &e) const;
  bool onKeyTypeEvent(const KeyTypeEvent &e) const;

private:
  INT64 g_Time = 0;
  INT64 g_TicksPerSecond = 0;
  debug::FrameTimingsWidget m_frameTimings;
  debug::MemoryConsumptionWidget m_memoryUsage;
  bool m_shouldShow = false;
  // 192 is the `
  static const uint32_t TRIGGER_UI_BUTTON = 192;
};
} // namespace SirEngine
