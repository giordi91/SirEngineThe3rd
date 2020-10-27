#pragma once

#include "SirEngine/layer.h"
#include "SirEngine/graphics/techniques/grass.h"

namespace SirEngine {

class ReloadScriptsEvent;
class DebugRenderConfigChanged;
class MouseButtonPressEvent;
class MouseButtonReleaseEvent;
class KeyboardReleaseEvent;
class MouseMoveEvent;
class WindowResizeEvent;
class DebugLayerChanged;
class ShaderCompileEvent;
class RenderSizeChanged;
struct Skeleton;
struct GraphAllocators;


class  GraphicsLayer final : public Layer {
public:
  GraphicsLayer() : Layer("VkTempLayer") {}
  ~GraphicsLayer() override = default;

  void allocateOffscreenBuffer(uint32_t w, uint32_t h);
  void onAttach() override;
  void onDetach() override;
  void onUpdate() override;
  void onEvent(Event &event) override;
  void clear() override;

private:
  // event implementation for the layer
  bool onMouseButtonPressEvent(MouseButtonPressEvent &e);
  bool onMouseButtonReleaseEvent(MouseButtonReleaseEvent &e);
  bool onMouseMoveEvent(MouseMoveEvent &e);
  bool onKeyboardReleaseEvent(KeyboardReleaseEvent &e);
  bool onDebugLayerEvent(DebugLayerChanged &e);
  bool onResizeEvent(WindowResizeEvent &e);
  bool onDebugConfigChanged(DebugRenderConfigChanged &e);
  bool onShaderCompileEvent(ShaderCompileEvent &e);
  bool onReloadScriptEvent(ReloadScriptsEvent &e);
  bool onRenderSizeChanged(RenderSizeChanged&e);
  void prepareRenderGraph();

  // camera event control
  bool leftDown = false;
  bool rightDown = false;
  bool middleDown = false;
  float previousX = 0;
  float previousY = 0;

  GraphAllocators *alloc;
  graphics::GrassTechnique m_grass;
  CommandBufferHandle m_workerBuffer{};
  RenderGraphContext m_graphContext{};
  TextureHandle offscreenBuffer{};
};
} // namespace SirEngine
