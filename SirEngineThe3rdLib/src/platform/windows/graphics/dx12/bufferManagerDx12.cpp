#include "platform/windows/graphics/dx12/bufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include <cassert>

namespace SirEngine::dx12 {
void BufferManagerDx12::free(const BufferHandle handle) { assert(0); }

BufferHandle BufferManagerDx12::allocate(const uint32_t sizeInByte,
                                         void *initData, const char *name,
                                         int numElements, int elementSize,
                                         bool isUAV) {
  ID3D12Resource *buffer = nullptr;

  // Create the actual default buffer resource.
  HRESULT res = dx12::DEVICE->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(
          sizeInByte, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&buffer));
  assert(SUCCEEDED(res));
  assert(initData == nullptr);

  // lets get a data from the pool
  uint32_t index;
  BufferData &data = m_bufferPool.getFreeMemoryData(index);

  // lets create the decriptor
  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferUAV(data.uav, data.data,
                                                 numElements, elementSize);

  // data is now loaded need to create handle etc
  BufferHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.data = buffer;
  data.state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  data.type = isUAV ? BufferType::UAV : BufferType::SRV;

  return handle;
}

void BufferManagerDx12::bindBuffer(
    const BufferHandle handle, const int slot,
    ID3D12GraphicsCommandList2 *commandList) const {

  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const BufferData &data = m_bufferPool.getConstRef(index);

  commandList->SetComputeRootDescriptorTable(slot, data.type == BufferType::UAV
                                                       ? data.uav.gpuHandle
                                                       : data.srv.gpuHandle);
}
} // namespace SirEngine::dx12
