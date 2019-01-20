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
}
void Graphics3DLayer::onDetach() {}
void Graphics3DLayer::onUpdate() {}
void Graphics3DLayer::onEvent(Event &event) {}
} // namespace SirEngine
