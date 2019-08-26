#pragma once

#include "SirEngine/identityManager.h"
#include "SirEngine/layer.h"
//TODO TEMP
#include "SirEngine/animation/animation_manager.h"
#include "SirEngine/handle.h"

namespace SirEngine {
class DebugRenderConfigChanged;
class MouseButtonPressEvent;
class MouseButtonReleaseEvent;
class MouseMoveEvent;
class WindowResizeEvent;
class DebugLayerChanged;
class ShaderCompileEvent;
struct Skeleton;

namespace dx12 {
class Texture2D;

} // namespace dx12
class SIR_ENGINE_API Graphics3DLayer final : public Layer {
public:
  Graphics3DLayer() : Layer("DX12GraphicsLayer") {}
  ~Graphics3DLayer() override = default;

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
  bool onDebugLayerEvent(DebugLayerChanged &e);
  bool onResizeEvent(WindowResizeEvent &e);
  bool onDebugConfigChanged(DebugRenderConfigChanged &e);
  bool onShaderCompileEvent(ShaderCompileEvent &e);

  // camera event control
  bool leftDown = false;
  bool rightDown = false;
  bool middleDown = false;
  float previousX = 0;
  float previousY = 0;

  // TODO temp
  Skeleton *m_skeleton;
  AnimationConfig m_config;
  DebugDrawHandle m_animHandle{};
};
} // namespace SirEngine
