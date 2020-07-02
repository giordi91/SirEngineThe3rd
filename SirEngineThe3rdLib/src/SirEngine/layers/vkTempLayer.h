#pragma once

#include "SirEngine/handle.h"
#include "SirEngine/layer.h"

namespace SirEngine {

class ReloadScriptsEvent;
class DebugRenderConfigChanged;
class MouseButtonPressEvent;
class MouseButtonReleaseEvent;
class MouseMoveEvent;
class WindowResizeEvent;
class DebugLayerChanged;
class ShaderCompileEvent;
struct Skeleton;
struct GraphAllocators;

namespace dx12 {
class Texture2D;

} // namespace dx12
class SIR_ENGINE_API VkTempLayer final : public Layer {
public:
  VkTempLayer() : Layer("VkTempLayer") {}
  ~VkTempLayer() override = default;

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
  bool onReloadScriptEvent(ReloadScriptsEvent &e);
  void initGrass();

  // camera event control
  bool leftDown = false;
  bool rightDown = false;
  bool middleDown = false;
  float previousX = 0;
  float previousY = 0;

  // TODO temp
  GraphAllocators *alloc;
  DebugDrawHandle m_debugHandle{};
  BufferHandle m_grassBuffer;
  MaterialHandle m_grassMaterial;
};
} // namespace SirEngine
