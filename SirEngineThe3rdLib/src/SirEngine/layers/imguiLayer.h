#pragma once

#include "SirEngine/debugUiWidgets/frameTimingsWidget.h"
#include "SirEngine/debugUiWidgets/hardwareInfo.h"
#include "SirEngine/debugUiWidgets/memoryConsumptionWidget.h"
#include "SirEngine/debugUiWidgets/renderGraphWidget.h"
#include "SirEngine/debugUiWidgets/shaderRecompileWidget.h"
#include "SirEngine/graphics/nodeGraph.h"
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

class SIR_ENGINE_API ImguiLayer : public Layer {
public:
  ImguiLayer() : Layer("ImGuiLayer"), m_renderGraph()
  {
  }

  ~ImguiLayer() override = default;

  void onAttach() override;
  void onDetach() override;
  void onUpdate() override;
  void onEvent(Event &event) override;
  void clear() override{};

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
  bool onRenderGraphEvent(const RenderGraphChanged &e);
  bool onCompileResultEvent(const ShaderCompileResultEvent &e);
  bool onRequestCompileEvent(const RequestShaderCompileEvent &e);

private:
  INT64 g_Time = 0;
  INT64 g_TicksPerSecond = 0;
  debug::FrameTimingsWidget m_frameTimings;
  debug::MemoryConsumptionWidget m_memoryUsage;
  debug::RenderGraphWidget m_renderGraph;
  debug::ShaderCompilerWidget m_shaderWidget;

#if BUILD_AMD
  debug::HWInfoWidget m_hwInfo;
#endif
  bool m_shouldShow = false;
  bool m_renderShaderCompiler = false;
  // 192 is the `
  static const uint32_t TRIGGER_UI_BUTTON = 192;
};
} // namespace SirEngine
