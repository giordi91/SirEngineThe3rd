#include "renderingContext.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"

namespace SirEngine {
void RenderingContext::initialize() {
  // ask for the camera buffer handle;
  m_cameraHandle =
      globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(CameraBuffer));
}
void RenderingContext::setupCameraForFrame() {
  globals::MAIN_CAMERA->updateCamera();
  m_camBufferCPU.vFov = 60.0f;
  m_camBufferCPU.screenWidth = static_cast<float>(globals::SCREEN_WIDTH);
  m_camBufferCPU.screenHeight = static_cast<float>(globals::SCREEN_HEIGHT);
  m_camBufferCPU.MVP = DirectX::XMMatrixTranspose(
      globals::MAIN_CAMERA->getMVP(DirectX::XMMatrixIdentity()));
  m_camBufferCPU.ViewMatrix = DirectX::XMMatrixTranspose(
      globals::MAIN_CAMERA->getViewInverse(DirectX::XMMatrixIdentity()));
  m_camBufferCPU.VPinverse= DirectX::XMMatrixTranspose(
      globals::MAIN_CAMERA->getMVPInverse(DirectX::XMMatrixIdentity()));
   m_camBufferCPU.perspectiveValues = globals::MAIN_CAMERA->getProjParams();
   DirectX::XMFLOAT3 camPos = globals::MAIN_CAMERA->getPosition();
  m_camBufferCPU.position = DirectX::XMFLOAT4{camPos.x, camPos.y, camPos.z, 1.0f};

  globals::CONSTANT_BUFFER_MANAGER->updateConstantBuffer(m_cameraHandle,
                                                         &m_camBufferCPU);
}
void RenderingContext::bindCameraBuffer(const int index) const {
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  commandList->SetGraphicsRootDescriptorTable(
      index,
      dx12::CONSTANT_BUFFER_MANAGER->getConstantBufferDx12Handle(m_cameraHandle)
          .gpuHandle);
}
} // namespace SirEngine
