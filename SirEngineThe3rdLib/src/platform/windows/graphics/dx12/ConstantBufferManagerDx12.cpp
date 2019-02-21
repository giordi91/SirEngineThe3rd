#include "platform/windows/graphics/dx12/constantBufferManagerDx12.h"
#include "d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"

namespace SirEngine {
namespace dx12 {
ConstantBufferManagerDx12::ConstantBufferManagerDx12() {
  for (auto &i : m_dynamicStorage) {
    i.reserve(RESERVE_SIZE);
  }
  m_descriptorStorage.reserve(RESERVE_SIZE);
}

ConstantBufferHandle
ConstantBufferManagerDx12::allocateDynamic(uint32_t sizeInBytes,
                                           void *inputData) {
  // must be at least 256 bytes
  uint32_t actualSize =
      sizeInBytes % 256 == 0 ? sizeInBytes : ((sizeInBytes / 256) + 1) * 256;

  ConstantBufferHandle handle{
      (MAGIC_NUMBER_COUNTER << 16) |
      (static_cast<uint32_t>(m_dynamicStorage[0].size()))};

  for (int i = 0; i < FRAME_BUFFERS_COUNT; ++i) {
    ConstantBufferData data;
    data.mapped = false;
    data.size = sizeInBytes;
    // D3DBuffer buffer;
    // create the upload buffer
    dx12::DEVICE->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(actualSize),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&data.resource));

    DescriptorPair pair;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferCBV(pair, data.resource,
                                                   actualSize);
    // map the buffer
    mapConstantBuffer(data);
    if (inputData != nullptr) {
      memcpy(data.mappedData, inputData, sizeInBytes);
    }

    data.descriptorIndex = m_descriptorStorage.size();
    data.magicNumber = MAGIC_NUMBER_COUNTER;
    m_dynamicStorage[i].push_back(data);
    m_descriptorStorage.push_back(pair);
    assert(data.resource != nullptr);
  }
  ++MAGIC_NUMBER_COUNTER;
  return handle;
}

void ConstantBufferManagerDx12::updateConstantBuffer(
    const ConstantBufferHandle handle, void *dataToUpload) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const ConstantBufferData &data =
      m_dynamicStorage[globals::CURRENT_FRAME][index];

  assert(data.mappedData != nullptr);
  memcpy(data.mappedData, dataToUpload, data.size);
}
} // namespace dx12
} // namespace SirEngine
