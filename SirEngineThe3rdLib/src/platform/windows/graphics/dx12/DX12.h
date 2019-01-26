#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

#include <cassert>

namespace SirEngine {
namespace dx12 {
class Adapter;
class DescriptorHeap;
class SwapChain;

struct FrameCommand final {
  ID3D12CommandAllocator *commandAllocator = nullptr;
#if DXR_ENABLED
  ID3D12GraphicsCommandList4 *commandList = nullptr;
#else
  ID3D12GraphicsCommandList3 *commandList = nullptr;
#endif
  bool isListOpen = false;
};

struct FrameResource final
{
	FrameCommand fc;
	UINT64 fence =0;
};

enum class DescriptorType {
  NONE = 0,
  CBV = 1,
  SRV = 2,
  UAV = 4,
  RTV = 8,
  DSV = 16
};

struct D3DBuffer {
  ID3D12Resource *resource;
  D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
  DescriptorType descriptorType;
};

inline HRESULT resetCommandList(FrameCommand *command) {

  assert(!command->isListOpen);
  HRESULT res = command->commandList->Reset(command->commandAllocator, nullptr);
  assert(SUCCEEDED(res));
  command->isListOpen = true;
  return res;
}

// should be used only at beginning of the frame
inline HRESULT resetAllocatorAndList(FrameCommand *command) {

  assert(!command->isListOpen);

  // Reuse the memory associated with command recording.
  // We can only reset when the associated command lists have finished
  // execution on the GPU.
  HRESULT result = command->commandAllocator->Reset();
  assert(SUCCEEDED(result) && "failed resetting allocator");

  // A command list can be reset after it has been added to the command queue
  // via ExecuteCommandList.
  // Reusing the command list reuses memory.
  HRESULT result2 =
      command->commandList->Reset(command->commandAllocator, nullptr);
  assert(SUCCEEDED(result) && "failed resetting allocator");
  command->isListOpen = SUCCEEDED(result) & SUCCEEDED(result2);
  return result2;
}

inline bool executeCommandList(ID3D12CommandQueue *queue,
                               FrameCommand *command) {
  assert(command->isListOpen);
  HRESULT res = command->commandList->Close();
  assert(SUCCEEDED(res) && "Error closing command list");
  ID3D12CommandList *commandLists[] = {command->commandList};
  queue->ExecuteCommandLists(1, commandLists);
  bool succeded = SUCCEEDED(res);
  assert(succeded);
  command->isListOpen = false;
  return succeded;
}

const int FRAME_BUFFERING_COUNT = 2;

namespace DX12Handles {
#if DXR_ENABLED
extern ID3D12Device5 *device;
#else
extern ID3D12Device4 *device;
#endif
extern ID3D12Debug *debugController;
extern IDXGIFactory6 *dxiFactory;
extern Adapter *adapter;
extern ID3D12CommandQueue *commandQueue;
extern UINT64 currentFence;
extern UINT64 currentFrame;
extern UINT64 TOTAL_CPU_FRAMES;
extern ID3D12Fence *fence;
extern DescriptorHeap *globalCBVSRVUAVheap;
extern DescriptorHeap *globalRTVheap;
extern DescriptorHeap *globalDSVheap;
extern SwapChain *swapChain;
extern FrameResource frameResources[FRAME_BUFFERING_COUNT];
extern FrameResource* currenFrameResource;
} // namespace DX12Handles

inline void flushCommandQueue(ID3D12CommandQueue *queue) {
  // Advance the fence value to mark commands up to this fence point.
  DX12Handles::currentFence++;

  // Add an instruction to the command queue to set a new fence point. Because
  // we are on the GPU time line, the new fence point won't be set until the
  // GPU finishes processing all the commands prior to this Signal().
  HRESULT res = queue->Signal(DX12Handles::fence, DX12Handles::currentFence);
  assert(SUCCEEDED(res));
  auto id = DX12Handles::fence->GetCompletedValue();
  // Wait until the GPU has completed commands up to this fence point.
  if (id < DX12Handles::currentFence) {
    HANDLE eventHandle =
        CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

    // Fire event when GPU hits current fence.
    res = DX12Handles::fence->SetEventOnCompletion(DX12Handles::currentFence,
                                                   eventHandle);
    assert(SUCCEEDED(res));

    // Wait until the GPU hits current fence event is fired.
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
  }
}

bool initializeGraphics();

} // namespace dx12
} // namespace SirEngine
