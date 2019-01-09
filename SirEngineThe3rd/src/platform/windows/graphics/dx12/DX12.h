#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

namespace SirEngine {
namespace dx12 {
class Adapter;
class DescriptorHeap;

struct CommandList {
  ID3D12CommandAllocator *commandAllocator = nullptr;
  ID3D12GraphicsCommandList4 *commandList = nullptr;
  bool isListOpen = false;
};

struct D3DBuffer {
  ID3D12Resource *resource;
  D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
};

inline HRESULT resetCommandList(CommandList *command) {

  assert(!command->isListOpen);
  HRESULT res = command->commandList->Reset(command->commandAllocator, nullptr);
  assert(SUCCEEDED(res));
  command->isListOpen = true;
  return res;
}

// should be used only at beginning of the frame
  inline HRESULT resetAllocatorAndList(CommandList* command) {

    assert(!command->isListOpen);

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished
    // execution on the GPU.
    HRESULT result = command->commandAllocator->Reset();
    assert(SUCCEEDED(result) && "failed resetting allocator");

    // A command list can be reset after it has been added to the command queue
    // via ExecuteCommandList.
    // Reusing the command list reuses memory.
    HRESULT result2 = command->commandList->Reset(command->commandAllocator, nullptr);
    assert(SUCCEEDED(result) && "failed resetting allocator");
    command->isListOpen= SUCCEEDED(result) & SUCCEEDED(result2);
    return result2;
  }

inline bool executeCommandList(ID3D12CommandQueue *queue,
                               CommandList *command) {
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

namespace DX12Handles {
extern ID3D12Device5 *device;
extern ID3D12Debug *debugController;
extern IDXGIFactory6 *dxiFactory;
extern Adapter *adapter;
extern ID3D12CommandQueue *commandQueue;
extern UINT64 currentFence;
extern ID3D12Fence *fence;
extern DescriptorHeap *globalCBVSRVUAVheap;
extern DescriptorHeap *globalRTVheap;
extern DescriptorHeap *globalDSVheap;
extern CommandList* commandList;
} // namespace DX12Handles

inline void flushCommandQueue(ID3D12CommandQueue *queue) {
  // Advance the fence value to mark commands up to this fence point.
  DX12Handles::currentFence++;

  // Add an instruction to the command queue to set a new fence point. Because
  // we are on the GPU timeline, the new fence point won't be set until the
  // GPU finishes processing all the commands prior to this Signal().
  HRESULT res = queue->Signal(DX12Handles::fence, DX12Handles::currentFence);
  auto id = DX12Handles::fence->GetCompletedValue();
  // Wait until the GPU has completed commands up to this fence point.
  if (id < DX12Handles::currentFence) {
    HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

    // Fire event when GPU hits current fence.
    res = DX12Handles::fence->SetEventOnCompletion(DX12Handles::currentFence,
                                                   eventHandle);

    // Wait until the GPU hits current fence event is fired.
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
  }
}

bool initializeGraphics();

} // namespace dx12
} // namespace SirEngine
