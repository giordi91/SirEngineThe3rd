#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

#include "SirEngine/globals.h"
#include <cassert>

namespace SirEngine {
namespace dx12 {
class TextureManager;
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

struct FrameResource final {
  FrameCommand fc;
  UINT64 fence = 0;
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

// global declarations
#if DXR_ENABLED
extern ID3D12Device5 *DEVICE;
#else
extern ID3D12Device4 *DEVICE;
#endif
extern ID3D12Debug *DEBUG_CONTROLLER;
extern IDXGIFactory6 *DXGI_FACTORY;
extern Adapter *ADAPTER;
extern UINT64 CURRENT_FENCE;
extern UINT64 CURRENT_FRAME;
extern DescriptorHeap *GLOBAL_CBV_SRV_UAV_HEAP;
extern DescriptorHeap *GLOBAL_RTV_HEAP;
extern DescriptorHeap *GLOBAL_DSV_HEAP;
extern ID3D12CommandQueue *GLOBAL_COMMAND_QUEUE;
extern ID3D12Fence *GLOBAL_FENCE;
extern SwapChain *SWAP_CHAIN;
extern FrameResource FRAME_RESOURCES[FRAME_BUFFERS_COUNT];
extern FrameResource *CURRENT_FRAME_RESOURCE;
extern TextureManager *TEXTURE_MANAGER;

inline void flushCommandQueue(ID3D12CommandQueue *queue) {
  // Advance the fence value to mark commands up to this fence point.
  CURRENT_FENCE++;

  // Add an instruction to the command queue to set a new fence point. Because
  // we are on the GPU time line, the new fence point won't be set until the
  // GPU finishes processing all the commands prior to this Signal().
  HRESULT res = queue->Signal(GLOBAL_FENCE, CURRENT_FENCE);
  assert(SUCCEEDED(res));
  auto id = GLOBAL_FENCE->GetCompletedValue();
  // Wait until the GPU has completed commands up to this fence point.
  if (id < CURRENT_FENCE) {
    HANDLE eventHandle =
        CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

    // Fire event when GPU hits current fence.
    res = GLOBAL_FENCE->SetEventOnCompletion(CURRENT_FENCE, eventHandle);
    assert(SUCCEEDED(res));

    // Wait until the GPU hits current fence event is fired.
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
  }
}

bool initializeGraphics();

} // namespace dx12
} // namespace SirEngine
