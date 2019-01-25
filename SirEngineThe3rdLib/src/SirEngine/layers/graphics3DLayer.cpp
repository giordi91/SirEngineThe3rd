#include "SirEngine/layers/graphics3DLayer.h"
#include "SirEngine/globals.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/swapChain.h"
#include <DirectXMath.h>

namespace SirEngine {

void Graphics3DLayer::onAttach() {
  Globals::mainCamera = new Camera3dPivot();
  Globals::mainCamera->setLookAt(0, 125, 0);
  Globals::mainCamera->setPosition(00, 125, 60);
  Globals::mainCamera->Render();
  m_camBuffer.initialize(dx12::DX12Handles::device,
                         dx12::DX12Handles::globalCBVSRVUAVheap,
                         sizeof(dx12::CameraBuffer), 0);
  m_camBuffer.map();

  if (!dx12::DX12Handles::frameCommand->isListOpen) {
    dx12::resetAllocatorAndList(dx12::DX12Handles::frameCommand);
  }
  m_mesh.loadFromFile(dx12::DX12Handles::device,
                      "data/processed/armorChest.model",
                      dx12::DX12Handles::globalCBVSRVUAVheap);

  dx12::executeCommandList(dx12::DX12Handles::commandQueue,
                           dx12::DX12Handles::frameCommand);
  dx12::flushCommandQueue(dx12::DX12Handles::commandQueue);

  m_shaderManager = new temp::rendering::ShaderManager();
  m_shaderManager->init();
  m_shaderManager->loadShadersInFolder("data/processed/shaders/rasterization");
  //m_shaderManager->loadShadersInFolder("data/shaders/rasterization");

  m_root = new temp::rendering::RootSignatureManager();
  m_root->init(dx12::DX12Handles::device);
  m_root->loadSingaturesInFolder("data/rs");

  m_reg = new temp::rendering::ShadersLayoutRegistry();

  m_pso = new temp::rendering::PSOManager();
  m_pso->init(dx12::DX12Handles::device, m_reg, m_root, m_shaderManager);
  m_pso->loadPSOInFolder("data/pso");
}
void Graphics3DLayer::onDetach() {}
void Graphics3DLayer::onUpdate() {

  if (!dx12::DX12Handles::frameCommand->isListOpen) {
    dx12::resetAllocatorAndList(dx12::DX12Handles::frameCommand);
  }

  auto commandList = dx12::DX12Handles::frameCommand->commandList;
  commandList->RSSetViewports(1, dx12::DX12Handles::swapChain->getViewport());
  commandList->RSSetScissorRects(
      1, dx12::DX12Handles::swapChain->getScissorRect());
  dx12::DX12Handles::swapChain->clearDepth();
  auto back = dx12::DX12Handles::swapChain->currentBackBufferView();
  auto depth = dx12::DX12Handles::swapChain->getDepthCPUDescriptor();

  commandList->OMSetRenderTargets(1, &back, true, &depth);
  Globals::mainCamera->Render();
  m_camBufferCPU.vFov = 60.0f;
  m_camBufferCPU.screenWidth = Globals::SCREEN_WIDTH;
  m_camBufferCPU.screenHeight = Globals::SCREEN_HEIGHT;
  m_camBufferCPU.MVP =
      DirectX::XMMatrixTranspose(Globals::mainCamera->getMVP(DirectX::XMMatrixIdentity()));
  m_camBuffer.update(&m_camBufferCPU);

  auto* pso = m_pso->getComputePSOByName("simpleMeshPSO.json");
  commandList->SetPipelineState(pso);
  auto* rs = m_root->getRootSignatureFromName("simpleMesh_RS");
  commandList->SetGraphicsRootSignature(rs);
  auto vview = m_mesh.getVertexBufferView();
  auto iview = m_mesh.getIndexBufferView();

  commandList->IASetIndexBuffer(&iview);
  commandList->IASetVertexBuffers(0,1,&vview);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->SetGraphicsRootDescriptorTable(0, m_camBuffer.getGPUView());
  commandList->DrawIndexedInstanced(m_mesh.getIndexCount(), 1, 0, 0, 0);

}
void Graphics3DLayer::onEvent(Event &event) {}
} // namespace SirEngine
