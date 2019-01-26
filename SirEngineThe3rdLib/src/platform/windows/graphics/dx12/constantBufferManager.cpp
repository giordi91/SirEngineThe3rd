#include "platform/windows/graphics/dx12/constantBufferManager.h"
#include "d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"

namespace SirEngine {
namespace dx12 {
ConstantBufferManager::ConstantBufferManager() {
  for (auto &i : m_dynamicStorage) {
    i.reserve(RESERVE_SIZE);
  }
  m_staticStorage.reserve(RESERVE_SIZE);
  m_descriptorStorage.reserve(RESERVE_SIZE);
}

ConstantBufferHandle
ConstantBufferManager::allocateDynamic(uint32_t sizeInBytes) {
  // must be at least 256 bytes
  uint32_t actualSize =
      sizeInBytes % 256 == 0 ? sizeInBytes : ((sizeInBytes / 256) + 1) * 256;

  ConstantBufferHandle handle;
  handle.handle = (MAGIC_NUMBER_COUNTER << 16) | (m_dynamicStorage[0].size());

  for (int i = 0; i < FRAME_BUFFERS_COUNT; ++i) {

    ConstantBufferData data;
	data.mapped = false;
    data.size = sizeInBytes;
    D3DBuffer buffer;
    // create the upload buffer
    dx12::DEVICE->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(actualSize),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&buffer.resource));

    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferCBV(&buffer,
                                                            actualSize);
    // map the buffer
    mapConstantBuffer(data, buffer);

    data.descriptorIndex = m_descriptorStorage.size();
    data.magicNumber = MAGIC_NUMBER_COUNTER;
    m_dynamicStorage[i].push_back(data);
    m_descriptorStorage.push_back(buffer);
    assert(buffer.resource != nullptr);
  }
  ++MAGIC_NUMBER_COUNTER;
  return handle;
}
} // namespace dx12
} // namespace SirEngine
