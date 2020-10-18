#pragma once

#include <imgui/imgui.h>

#include "SirEngine/layer.h"

namespace SirEngine {
class Event;
class MouseMoveEvent;
class KeyTypeEvent;
class KeyboardReleaseEvent;
class WindowResizeEvent;
class KeyboardPressEvent;
class MouseScrollEvent;
class MouseButtonReleaseEvent;
class MouseButtonPressEvent;
class RenderGraphChanged;
class ShaderCompileResultEvent;
class RequestShaderCompileEvent;

class EditorLayer final : public Layer {
  struct DockIDs {
    ImGuiID root = 0;
    ImGuiID bottom = 0;
    ImGuiID left = 0;
    ImGuiID right = 0;
    ImGuiID center = 0;
  };

 public:
  EditorLayer() : Layer("EditorLayer") {}

  ~EditorLayer() override = default;

  void onAttach() override;
  void onDetach() override;
  void onUpdate() override;
  void onEvent(Event &event) override;
  void clear() override;

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

  void setupDockSpaceLayout(int width, int height);

 private:
  uint64_t g_Time = 0;
  uint64_t g_TicksPerSecond = 0;
  bool m_shouldShow = true;
  DockIDs dockIds;

};
}  // namespace SirEngine
