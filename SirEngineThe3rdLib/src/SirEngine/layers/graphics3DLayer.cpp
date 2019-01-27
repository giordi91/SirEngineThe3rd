#include "SirEngine/layers/graphics3DLayer.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/swapChain.h"
#include <DirectXMath.h>

namespace SirEngine {

void Graphics3DLayer::onAttach() {
  Globals::MAIN_CAMERA = new Camera3DPivot();
  Globals::MAIN_CAMERA->setLookAt(0, 125, 0);
  Globals::MAIN_CAMERA->setPosition(00, 125, 60);
  Globals::MAIN_CAMERA->updateCamera();

  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;

  if (!currentFc->isListOpen) {
    dx12::resetAllocatorAndList(currentFc);
  }
  m_mesh.loadFromFile(dx12::DEVICE, "data/processed/meshes/armorChest.model",
                      dx12::GLOBAL_CBV_SRV_UAV_HEAP);

  dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE, currentFc);
  dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE);

  m_shaderManager = new SirEngine::dx12::ShaderManager();
  m_shaderManager->init();
  m_shaderManager->loadShadersInFolder("data/processed/shaders/rasterization");

  m_root = new dx12::RootSignatureManager();
  m_root->loadSingaturesInFolder("data/processed/rs");

  m_reg = new dx12::ShadersLayoutRegistry();

  m_pso = new temp::rendering::PSOManager();
  m_pso->init(dx12::DEVICE, m_reg, m_root, m_shaderManager);
  m_pso->loadPSOInFolder("data/pso");

  // ask for the camera buffer handle;
  m_cameraHandle =
      m_constantBufferManager.allocateDynamic(sizeof(dx12::CameraBuffer));
}
void Graphics3DLayer::onDetach() {}
void Graphics3DLayer::onUpdate() {

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  if (!currentFc->isListOpen) {
    dx12::resetAllocatorAndList(currentFc);
  }

  auto commandList = currentFc->commandList;
  commandList->RSSetViewports(1, dx12::SWAP_CHAIN->getViewport());
  commandList->RSSetScissorRects(1, dx12::SWAP_CHAIN->getScissorRect());
  dx12::SWAP_CHAIN->clearDepth();
  auto back = dx12::SWAP_CHAIN->currentBackBufferView();
  auto depth = dx12::SWAP_CHAIN->getDepthCPUDescriptor();

  commandList->OMSetRenderTargets(1, &back, true, &depth);

  Globals::MAIN_CAMERA->updateCamera();
  m_camBufferCPU.vFov = 60.0f;
  m_camBufferCPU.screenWidth = static_cast<float>(Globals::SCREEN_WIDTH);
  m_camBufferCPU.screenHeight = static_cast<float>(Globals::SCREEN_HEIGHT);
  m_camBufferCPU.mvp = DirectX::XMMatrixTranspose(
      Globals::MAIN_CAMERA->getMVP(DirectX::XMMatrixIdentity()));

  m_constantBufferManager.updateConstantBuffer(m_cameraHandle, &m_camBufferCPU);

  auto *pso = m_pso->getComputePSOByName("simpleMeshPSO");
  commandList->SetPipelineState(pso);
  auto *rs = m_root->getRootSignatureFromName("simpleMeshRS");
  commandList->SetGraphicsRootSignature(rs);
  auto vview = m_mesh.getVertexBufferView();
  auto iview = m_mesh.getIndexBufferView();

  commandList->IASetIndexBuffer(&iview);
  commandList->IASetVertexBuffers(0, 1, &vview);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  commandList->SetGraphicsRootDescriptorTable(
      0, m_constantBufferManager.getConstantBufferDescriptor(m_cameraHandle)
             .gpuDescriptorHandle);

  commandList->DrawIndexedInstanced(m_mesh.getIndexCount(), 1, 0, 0, 0);
}
void Graphics3DLayer::onEvent(Event &event) {

  EventDispatcher dispatcher(event);
  dispatcher.dispatch<MouseButtonPressEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onMouseButtonPressEvent));
  dispatcher.dispatch<MouseButtonReleaseEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onMouseButtonReleaseEvent));
  dispatcher.dispatch<MouseMoveEvent>(
      SE_BIND_EVENT_FN(Graphics3DLayer::onMouseMoveEvent));
}

bool Graphics3DLayer::onMouseButtonPressEvent(MouseButtonPressEvent &e) {
  if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::LEFT) {
    leftDown = true;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::RIGHT) {
    rightDown = true;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::MIDDLE) {
    middleDown = true;
    return true;
  }
  return false;
}

bool Graphics3DLayer::onMouseButtonReleaseEvent(MouseButtonReleaseEvent &e) {
  if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::LEFT) {
    leftDown = false;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::RIGHT) {
    rightDown = false;
    return true;
  } else if (e.getMouseButton() == MOUSE_BUTTONS_EVENT::MIDDLE) {
    middleDown = false;
    return true;
  }
  return false;
}

bool Graphics3DLayer::onMouseMoveEvent(MouseMoveEvent &e) {
  SE_CORE_INFO("{0} {1}, {2}, {3}", e.getX(), e.getY(), previousX, previousY);
  const float deltaX = previousX - e.getX();
  const float deltaY = previousY - e.getY();
  if (leftDown) {
    Globals::MAIN_CAMERA->rotCamera(deltaX, deltaY);
  } else if (middleDown) {
    Globals::MAIN_CAMERA->panCamera(deltaX, deltaY);
  } else if (rightDown) {
    Globals::MAIN_CAMERA->zoomCamera(deltaX);
  }

  // storing old position
  previousX = e.getX();
  previousY = e.getY();
  return true;
}

} // namespace SirEngine
