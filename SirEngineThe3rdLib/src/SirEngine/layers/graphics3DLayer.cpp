#include "SirEngine/layers/graphics3DLayer.h"
#include "SirEngine/globals.h"
#include "platform/windows/graphics/dx12/DX12.h"

namespace SirEngine {
void Graphics3DLayer::onAttach() {
  Globals::mainCamera = new Camera3dPivot();

  if (!dx12::DX12Handles::frameCommand->isListOpen)
  {
	  dx12::resetAllocatorAndList(dx12::DX12Handles::frameCommand);
  }
  m_mesh.loadFromFile(dx12::DX12Handles::device,
                      "data/processed/armorChest.model",
                      dx12::DX12Handles::globalCBVSRVUAVheap);

  dx12::executeCommandList(dx12::DX12Handles::commandQueue, dx12::DX12Handles::frameCommand);
  dx12::flushCommandQueue(dx12::DX12Handles::commandQueue);

  m_shaderManager = new temp::rendering::ShaderManager();
  m_shaderManager->init();
  m_shaderManager->loadShadersInFolder("data/shaders/rasterization");
 
  m_root = new temp::rendering::RootSignatureManager();
  m_root->init(dx12::DX12Handles::device);
  m_root->loadSingaturesInFolder("data/rs");

  m_reg = new temp::rendering::ShadersLayoutRegistry();

  m_pso = new temp::rendering::PSOManager();
  m_pso->init(dx12::DX12Handles::device, m_reg, m_root, m_shaderManager);
  m_pso->loadPSOInFolder("data/pso");




}
void Graphics3DLayer::onDetach() {}
void Graphics3DLayer::onUpdate() {}
void Graphics3DLayer::onEvent(Event &event) {}
} // namespace SirEngine
