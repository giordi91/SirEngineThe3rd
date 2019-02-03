#include "SirEngine/graphics/graphicsCore.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/swapChain.h"
#include "platform/windows/graphics/dx12/textureManager.h"
#include <d3d12.h>

namespace SirEngine {
namespace graphics {
void onResize(const uint32_t width, const uint32_t height) {
  dx12::SWAP_CHAIN->resize(&dx12::CURRENT_FRAME_RESOURCE->fc, width, height);
}
void newFrame() {

  // TODO clear here, there should be no specific dx12 stuff
  // here we need to check which frame resource we are going to use
  dx12::CURRENT_FRAME = (dx12::CURRENT_FRAME + 1) % FRAME_BUFFERS_COUNT;
  dx12::CURRENT_FRAME_RESOURCE = &dx12::FRAME_RESOURCES[dx12::CURRENT_FRAME];

  // check if the resource has finished rendering if not we have to wait
  if (dx12::CURRENT_FRAME_RESOURCE->fence != 0 &&
      dx12::GLOBAL_FENCE->GetCompletedValue() <
          dx12::CURRENT_FRAME_RESOURCE->fence) {
    HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
    auto handleResult = dx12::GLOBAL_FENCE->SetEventOnCompletion(
        dx12::CURRENT_FRAME_RESOURCE->fence, eventHandle);
    assert(SUCCEEDED(handleResult));
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
  }
  // at this point we know we are ready to go

  // Clear the back buffer and depth buffer.
  float gray[4] = {0.5f, 0.9f, 0.5f, 1.0f};
  // Reuse the memory associated with command recording.
  // We can only reset when the associated command lists have finished
  // execution on the GPU.
  resetAllocatorAndList(&dx12::CURRENT_FRAME_RESOURCE->fc);
  // Indicate a state transition on the resource usage.
  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;
  D3D12_RESOURCE_BARRIER rtbarrier[1];

  dx12::TextureHandle backBufferH =
      dx12::SWAP_CHAIN->currentBackBufferTexture();
  int rtcounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      backBufferH, D3D12_RESOURCE_STATE_RENDER_TARGET, rtbarrier, 0);
  if (rtcounter != 0) {
    commandList->ResourceBarrier(rtcounter, rtbarrier);
  }

  // Set the viewport and scissor rect.  This needs to be reset whenever the
  // command list is reset.
  commandList->RSSetViewports(1, dx12::SWAP_CHAIN->getViewport());
  commandList->RSSetScissorRects(1, dx12::SWAP_CHAIN->getScissorRect());

  // Clear the back buffer and depth buffer.
  commandList->ClearRenderTargetView(dx12::SWAP_CHAIN->currentBackBufferView(),
                                     gray, 0, nullptr);

  dx12::SWAP_CHAIN->clearDepth();
  dx12::SwapChain *swapChain = dx12::SWAP_CHAIN;
  // Specify the buffers we are going to render to.
  auto back = swapChain->currentBackBufferView();
  auto depth = swapChain->getDepthCPUDescriptor();
  commandList->OMSetRenderTargets(1, &back, true, &depth);
  auto *heap = dx12::GLOBAL_CBV_SRV_UAV_HEAP->getResource();
  commandList->SetDescriptorHeaps(1, &heap);
}
void dispatchFrame() {

  D3D12_RESOURCE_BARRIER rtbarrier[1];
  // finally transition the resource to be present
  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;

  dx12::TextureHandle backBufferH =
      dx12::SWAP_CHAIN->currentBackBufferTexture();
  int rtcounter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      backBufferH, D3D12_RESOURCE_STATE_PRESENT, rtbarrier, 0);
  if (rtcounter != 0) {
    commandList->ResourceBarrier(rtcounter, rtbarrier);
  }

  // Done recording commands.
  dx12::executeCommandList(dx12::GLOBAL_COMMAND_QUEUE,
                           &dx12::CURRENT_FRAME_RESOURCE->fc);

  dx12::CURRENT_FRAME_RESOURCE->fence = ++dx12::CURRENT_FENCE;
  dx12::GLOBAL_COMMAND_QUEUE->Signal(dx12::GLOBAL_FENCE, dx12::CURRENT_FENCE);
  // swap the back and front buffers
  dx12::SWAP_CHAIN->present();
}

void shutdown() { dx12::flushCommandQueue(dx12::GLOBAL_COMMAND_QUEUE); }
} // namespace graphics
} // namespace SirEngine
