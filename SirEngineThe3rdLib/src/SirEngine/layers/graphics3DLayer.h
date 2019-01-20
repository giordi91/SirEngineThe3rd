#pragma once

//#include "SirEngine/events/applicationEvent.h"
//#include "SirEngine/events/keyboardEvent.h"
//#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/graphics/camera.h"
#include "SirEngine/layer.h"
#include "platform/windows/graphics/dx12/mesh.h"
namespace SirEngine {

namespace dx12 {
struct D3DBuffer;
}
class SIR_ENGINE_API Graphics3DLayer : public Layer {
public:
  Graphics3DLayer() : Layer("DX12GraphicsLayer") {}
  ~Graphics3DLayer() override = default;

  void onAttach() override;
  void onDetach() override;
  void onUpdate() override;
  void onEvent(Event &event) override;

private:
  // event implementation for the layer
  // bool OnMouseButtonPressEvent(MouseButtonPressEvent &e);
  // bool OnMouseButtonReleaseEvent(MouseButtonReleaseEvent &e);
  // bool OnMouseMoveEvent(MouseMoveEvent &e);
  // bool OnMouseScrolledEvent(MouseScrollEvent &e);
  // bool OnKeyPressedEvent(KeyboardPressEvent &e);
  // bool OnKeyReleasedEvent(KeyboardReleaseEvent &e);
  // bool OnWindowResizeEvent(WindowResizeEvent &e);
  // bool OnKeyTypeEvent(KeyTypeEvent &e);
  Camera3dPivot *m_camera;
  dx12::Dx12RaytracingMesh m_mesh;

};
} // namespace SirEngine
