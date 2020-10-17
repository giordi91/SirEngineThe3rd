#include "platform/windows/graphics/dx12/dx12CommandBufferManager.h"

#include "DX12.h"
#include "SirEngine/runtimeString.h"

namespace SirEngine::dx12 {

CommandBufferHandle Dx12CommandBufferManager::createBuffer(
    COMMAND_BUFFER_ALLOCATION_FLAGS flags, const char *name) {
  uint32_t index;
  Dx12CommandBufferData &data = m_bufferPool.getFreeMemoryData(index);
  auto result = DEVICE->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&data.commandAllocator));
  assert(SUCCEEDED(result));

  result = DEVICE->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                     data.commandAllocator, nullptr,
                                     IID_PPV_ARGS(&data.commandList));
  assert(SUCCEEDED(result));
  DX_SET_DEBUG_NAME(data.commandList,
                    frameConcatenation(name, "CommandBuffer"));
  DX_SET_DEBUG_NAME(data.commandAllocator,
                    frameConcatenation(name, "CommandAllocator"));

  data.commandList->Close();
  data.isListOpen = false;
  data.version = m_versionCounter++;

  return {data.version << 16 | index};
}

void Dx12CommandBufferManager::executeBuffer(const CommandBufferHandle handle) {
  assertVersion(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  auto &data = m_bufferPool[idx];
  assert(data.isListOpen);
  HRESULT res = data.commandList->Close();
  assert(SUCCEEDED(res) && "Error closing command list");
  ID3D12CommandList *commandLists[] = {data.commandList};
  dx12::GLOBAL_COMMAND_QUEUE->ExecuteCommandLists(1, commandLists);
  bool succeded = SUCCEEDED(res);
  assert(succeded);
  data.isListOpen = false;
}

void Dx12CommandBufferManager::resetBufferHandle(const CommandBufferHandle handle) {
  assertVersion(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  auto &data = m_bufferPool[idx];
  assert(!data.isListOpen);

  // Reuse the memory associated with command recording.
  // We can only reset when the associated command lists have finished
  // execution on the GPU.
  HRESULT result = data.commandAllocator->Reset();
  assert(SUCCEEDED(result) && "failed resetting allocator");

  // A command list can be reset after it has been added to the command queue
  // via ExecuteCommandList.
  // Reusing the command list reuses memory.
  HRESULT result2 = data.commandList->Reset(data.commandAllocator, nullptr);
  assert(SUCCEEDED(result) && "failed resetting allocator");
  data.isListOpen = SUCCEEDED(result) & SUCCEEDED(result2);
}

void Dx12CommandBufferManager::executeFlushAndReset(
    const CommandBufferHandle handle) {
  executeBuffer(handle);
  globals::RENDERING_CONTEXT->flush();
  resetBufferHandle(handle);
}

void Dx12CommandBufferManager::freeBuffer(const CommandBufferHandle handle) {
  assertVersion(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  auto &data = m_bufferPool[idx];
  data.commandList->Release();
  data.commandAllocator->Release();
  m_bufferPool.free(idx);
}
}  // namespace SirEngine::dx12
