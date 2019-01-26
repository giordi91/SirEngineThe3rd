#include "SirEngine/graphics/graphicsCore.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/barrierUtils.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/swapChain.h"
#include <d3d12.h>

namespace SirEngine {
namespace graphics {
void onResize(unsigned int width, unsigned int height) {

  dx12::DX12Handles::swapChain->resize(
      &dx12::DX12Handles::currenFrameResource->fc, width, height);
}
void newFrame() {

  // TODO clear here, there should be no specific dx12 stuff
  // here we need to check which frame resource we are going to use
  dx12::DX12Handles::currentFrame =
      (dx12::DX12Handles::currentFrame + 1) % dx12::FRAME_BUFFERING_COUNT;
  dx12::DX12Handles::currenFrameResource =
      &dx12::DX12Handles::frameResources[dx12::DX12Handles::currentFrame];

  // check if the resource has finished rendering if not we have to wait
  if (dx12::DX12Handles::currenFrameResource->fence != 0 &&
      dx12::DX12Handles::fence->GetCompletedValue() <
          dx12::DX12Handles::currenFrameResource->fence) {
    HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
    auto handleResult = dx12::DX12Handles::fence->SetEventOnCompletion(
        dx12::DX12Handles::currenFrameResource->fence, eventHandle);
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
  }
  // at this point we know we are ready to go

  // Clear the back buffer and depth buffer.
  float gray[4] = {0.5f, 0.9f, 0.5f, 1.0f};
  // Reuse the memory associated with command recording.
  // We can only reset when the associated command lists have finished
  // execution on the GPU.
  resetAllocatorAndList(&dx12::DX12Handles::currenFrameResource->fc);
  // Indicate a state transition on the resource usage.
  auto *commandList = dx12::DX12Handles::currenFrameResource->fc.commandList;
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
  auto *commandList = dx12::DX12Handles::currenFrameResource->fc.commandList;
  commandList->ResourceBarrier(rtcounter, rtbarrier);

  // Done recording commands.
  dx12::executeCommandList(dx12::DX12Handles::commandQueue,
                           &dx12::DX12Handles::currenFrameResource->fc);

  dx12::DX12Handles::currenFrameResource->fence =
      ++dx12::DX12Handles::currentFence;
  dx12::DX12Handles::commandQueue->Signal(dx12::DX12Handles::fence,
                                          dx12::DX12Handles::currentFence);
  // swap the back and front buffers
  dx12::DX12Handles::swapChain->present();

  // Wait until frame commands are complete.  This waiting is inefficient and
  // is done for simplicity.  Later we will show how to organize our rendering
  // code so we do not have to wait per frame.
}
} // namespace graphics
} // namespace SirEngine
