#pragma once

#include "SirEngine/events/applicationEvent.h"
#include "SirEngine/events/keyboardEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/layer.h"

namespace SirEngine {

namespace dx12 {
struct D3DBuffer;
}
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
  bool OnMouseButtonPressEvent(MouseButtonPressEvent &e);
  bool OnMouseButtonReleaseEvent(MouseButtonReleaseEvent &e);
  bool OnMouseMoveEvent(MouseMoveEvent &e);
  bool OnMouseScrolledEvent(MouseScrollEvent &e);
  bool OnKeyPressedEvent(KeyboardPressEvent &e);
  bool OnKeyReleasedEvent(KeyboardReleaseEvent &e);
  bool OnWindowResizeEvent(WindowResizeEvent &e);
  bool OnKeyTypeEvent(KeyTypeEvent &e);

private:
  dx12::D3DBuffer *m_fontTextureDescriptor = nullptr;
  int m_descriptorIndex;

  INT64 g_Time = 0;
  INT64 g_TicksPerSecond = 0;
};
} // namespace SirEngine
