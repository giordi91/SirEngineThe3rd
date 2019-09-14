#include "platform/windows/graphics/dx12/bufferManagerDx12.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include <cassert>

namespace SirEngine::dx12 {
void BufferManagerDx12::free(const BufferHandle handle) {

  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  BufferData &data = m_bufferPool[index];
  data.data->Release();

  if (data.srv.cpuHandle.ptr != 0) {
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(data.srv);
  }
  if (data.uav.cpuHandle.ptr != 0) {
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(data.uav);
  }
}

BufferHandle BufferManagerDx12::allocate(const uint32_t sizeInByte,
                                         void *initData, const char *name,
                                         int numElements, int elementSize,
                                         bool isUAV) {
  ID3D12Resource *buffer = nullptr;
  ID3D12Resource *uploadBuffer = nullptr;

  // must be at least 256 bytes
  uint32_t actualSize =
      sizeInByte % 256 == 0 ? sizeInByte : ((sizeInByte / 256) + 1) * 256;

  auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
      actualSize, isUAV ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                        : D3D12_RESOURCE_FLAG_NONE);
  // Create the actual default buffer resource.
  HRESULT res = dx12::DEVICE->CreateCommittedResource(
      &heap, D3D12_HEAP_FLAG_NONE, &bufferDesc,
      isUAV ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
            : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                  D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
      nullptr, IID_PPV_ARGS(&buffer));
  assert(SUCCEEDED(res));
  assert(initData == nullptr);

  if (initData != nullptr) {

    // In order to copy CPU memory data into our default buffer, we need to
    // create an intermediate upload heap.
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(actualSize);
    res = dx12::DEVICE->CreateCommittedResource(
        &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&uploadBuffer));
    assert(SUCCEEDED(res));

    // Describe the data we want to copy into the default buffer.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = actualSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Schedule to copy the data to the default buffer resource.  At a high
    // level, the helper function UpdateSubresources will copy the CPU memory
    // into the intermediate upload heap.  Then, using
    // ID3D12CommandList::CopySubresourceRegion, the intermediate upload heap
    // data will be copied to mBuffer.
    auto preTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    auto commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;
    dx12::CURRENT_FRAME_RESOURCE->fc.commandList->ResourceBarrier(
        1, &preTransition);
    UpdateSubresources<1>(commandList, buffer, uploadBuffer, 0, 0, 1,
                          &subResourceData);
    auto postTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        buffer, D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ);
    commandList->ResourceBarrier(1, &postTransition);

    //// Note: uploadBuffer has to be kept alive after the above function calls
    //// because the command list has not been executed yet that performs the
    //// actual copy. The caller can Release the uploadBuffer after it knows the
    //// copy has been executed.
    //return defaultBuffer;
  }

  // lets get a data from the pool
  uint32_t index;
  BufferData &data = m_bufferPool.getFreeMemoryData(index);

  if (isUAV) {
    // lets create the decriptor
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferUAV(data.uav, data.data,
                                                   numElements, elementSize);
  } else {
    assert(0);
  }

  // data is now loaded need to create handle etc
  BufferHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.data = buffer;
  data.state = isUAV ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                     : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                           D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  data.type = isUAV ? BufferType::UAV : BufferType::SRV;

  return handle;
}

void BufferManagerDx12::bindBuffer(
    const BufferHandle handle, const int slot,
    ID3D12GraphicsCommandList2 *commandList) const {

  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const BufferData &data = m_bufferPool.getConstRef(index);

  D3D12_GPU_DESCRIPTOR_HANDLE toBind =
      data.type == BufferType::UAV ? data.uav.gpuHandle : data.srv.gpuHandle;
  // commandList->SetComputeRootDescriptorTable(slot, toBind);
  commandList->SetComputeRootUnorderedAccessView(
      slot, data.data->GetGPUVirtualAddress());
}
void BufferManagerDx12::bindBufferAsSRVGraphics(
    const BufferHandle handle, const int slot,
    ID3D12GraphicsCommandList2 *commandList) const {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const BufferData &data = m_bufferPool.getConstRef(index);

  commandList->SetGraphicsRootShaderResourceView(
      slot, data.data->GetGPUVirtualAddress());
}
} // namespace SirEngine::dx12
