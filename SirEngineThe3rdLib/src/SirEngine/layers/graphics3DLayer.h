#pragma once

#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/layer.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/constantBufferManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/shaderLayout.h"
#include "platform/windows/graphics/dx12/shaderManager.h"
#include "platform/windows/graphics/dx12/textureManager.h"
#include "platform/windows/graphics/dx12/meshManager.h"
#include <directxmath.h>

namespace SirEngine {

namespace dx12 {
class Texture2D;

struct CameraBuffer final {
  DirectX::XMMATRIX mvp;
  DirectX::XMMATRIX viewMatrix;
  float vFov;
  float screenWidth;
  float screenHeight;
  float padding;
};
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

  // member variable mostly temporary
  //dx12::Mesh m_mesh;
  dx12::CameraBuffer m_camBufferCPU{};
  dx12::ShaderManager *m_shaderManager{};
  dx12::RootSignatureManager *m_root{};
  dx12::ShadersLayoutRegistry *m_reg{};
  temp::rendering::PSOManager *m_pso{};
  dx12::ConstantBufferHandle m_cameraHandle{};
  dx12::ConstantBufferManager m_constantBufferManager;


  dx12::MeshHandle meshHandle;
  uint32_t meshIndexCount =0 ;
  // camera event control
  bool leftDown = false;
  bool rightDown = false;
  bool middleDown = false;
  float previousX = 0;
  float previousY = 0;

  dx12::TextureHandle th;
  dx12::DescriptorPair thSRV;

};
} // namespace SirEngine
