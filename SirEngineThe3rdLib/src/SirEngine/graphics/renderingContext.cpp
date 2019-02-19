#include "renderingContext.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"

namespace SirEngine {
void RenderingContext::initialize() {
  // ask for the camera buffer handle;
  m_cameraHandle = globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(
      sizeof(CameraBuffer));

}
void RenderingContext::setupCameraForFrame()
{
  globals::MAIN_CAMERA->updateCamera();
  m_camBufferCPU.vFov = 60.0f;
  m_camBufferCPU.screenWidth = static_cast<float>(globals::SCREEN_WIDTH);
  m_camBufferCPU.screenHeight = static_cast<float>(globals::SCREEN_HEIGHT);
  m_camBufferCPU.mvp = DirectX::XMMatrixTranspose(
      globals::MAIN_CAMERA->getMVP(DirectX::XMMatrixIdentity()));

  globals::CONSTANT_BUFFER_MANAGER->updateConstantBuffer(m_cameraHandle,
                                                         &m_camBufferCPU);

}
void RenderingContext::bindCameraBuffer(int index)
{
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  commandList->SetGraphicsRootDescriptorTable(
      index,
      // TODO remove this, wrap it into a context maybe and remove graphics
      // core?
      dx12::CONSTANT_BUFFER_MANAGER->getConstantBufferDx12Handle(m_cameraHandle)
          .gpuHandle);
}
} // namespace SirEngine
