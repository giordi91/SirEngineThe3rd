#include "SirEngine/graphics/graphicsCore.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/barrierUtils.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/swapChain.h"

namespace SirEngine {
namespace graphics {
void onResize(unsigned int width, unsigned int height) {

  dx12::DX12Handles::swapChain->resize(dx12::DX12Handles::commandList, width,
                                       height);
}
void newFrame() {
  // Clear the back buffer and depth buffer.
  float gray[4] = {0.5f, 0.9f, 0.5f, 1.0f};
  // Reuse the memory associated with command recording.
  // We can only reset when the associated command lists have finished execution
  // on the GPU.
  resetAllocatorAndList(dx12::DX12Handles::commandList);
  // Indicate a state transition on the resource usage.
  auto *commandList = dx12::DX12Handles::commandList->commandList;
  D3D12_RESOURCE_BARRIER rtbarrier[1];

  int rtcounter = dx12::transitionTexture2DifNeeded(
      dx12::DX12Handles::swapChain->currentBackBuffer(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, rtbarrier, 0);
  if (rtcounter != 0) {
    commandList->ResourceBarrier(rtcounter, rtbarrier);
  }

  // Set the viewport and scissor rect.  This needs to be reset whenever the
  // command list is reset.
  commandList->RSSetViewports(1, dx12::DX12Handles::swapChain->getViewport());
  commandList->RSSetScissorRects(
      1, dx12::DX12Handles::swapChain->getScissorRect());

  // Clear the back buffer and depth buffer.
  commandList->ClearRenderTargetView(
      dx12::DX12Handles::swapChain->currentBackBufferView(), gray, 0, nullptr);

  dx12::DX12Handles::swapChain->clearDepth();
  dx12::SwapChain *swapChain = dx12::DX12Handles::swapChain;
  // Specify the buffers we are going to render to.
  auto back = swapChain->currentBackBufferView();
  auto depth = swapChain->getDepthCPUDescriptor();
  commandList->OMSetRenderTargets(1, &back, true, &depth);
  //&m_depthStencilBufferResource.cpuDescriptorHandle);
  auto *heap = dx12::DX12Handles::globalCBVSRVUAVheap->getResource();
  commandList->SetDescriptorHeaps(1, &heap);
}
void dispatchFrame() {

  D3D12_RESOURCE_BARRIER rtbarrier[1];
  // finally transition the resource to be present
  int rtcounter = dx12::transitionTexture2D(
      dx12::DX12Handles::swapChain->currentBackBuffer(),
      D3D12_RESOURCE_STATE_PRESENT, rtbarrier, 0);
  dx12::DX12Handles::commandList->commandList->ResourceBarrier(rtcounter,
                                                               rtbarrier);

  // Done recording commands.
  dx12::executeCommandList(dx12::DX12Handles::commandQueue,
                           dx12::DX12Handles::commandList);

  // swap the back and front buffers
  dx12::DX12Handles::swapChain->present();

  // Wait until frame commands are complete.  This waiting is inefficient and is
  // done for simplicity.  Later we will show how to organize our rendering code
  // so we do not have to wait per frame.
  dx12::flushCommandQueue(dx12::DX12Handles::commandQueue);
}
} // namespace graphics
} // namespace SirEngine
