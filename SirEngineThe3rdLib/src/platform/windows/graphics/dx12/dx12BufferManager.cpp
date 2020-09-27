#include "platform/windows/graphics/dx12/dx12BufferManager.h"

#include <cassert>

#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"
#include "d3dx12.h"
#include "descriptorHeap.h"
#include "platform/windows/graphics/dx12/DX12.h"

namespace SirEngine::dx12 {
ID3D12Resource *BufferManagerDx12::getNativeBuffer(
    const BufferHandle bufferHandle) {
  assertMagicNumber(bufferHandle);
  uint32_t index = getIndexFromHandle(bufferHandle);
  BufferData &data = m_bufferPool[index];
  return data.data;
}

void BufferManagerDx12::free(const BufferHandle handle) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  BufferData &data = m_bufferPool[index];
  data.data->Release();
}

ID3D12Resource *BufferManagerDx12::allocateCpuVisibleBuffer(
    const uint32_t actualSize) const {
  ID3D12Resource *uploadBuffer;
  // In order to copy CPU memory data into our default buffer, we need to
  // create an intermediate upload heap.
  auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(actualSize);
  HRESULT res = dx12::DEVICE->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
  assert(SUCCEEDED(res));

  return uploadBuffer;
}

ID3D12Resource *BufferManagerDx12::allocateGpuVisibleBuffer(
    const uint32_t actualSize, const bool isUav) {
  ID3D12Resource *buffer = nullptr;
  auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
      actualSize, isUav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                        : D3D12_RESOURCE_FLAG_NONE);
  HRESULT res = dx12::DEVICE->CreateCommittedResource(
      &heap, D3D12_HEAP_FLAG_NONE, &bufferDesc,
      isUav ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
            : D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&buffer));

  assert(SUCCEEDED(res));
  return buffer;
}

void BufferManagerDx12::uploadDataToGpuOnlyBuffer(void *initData,
                                                  const uint32_t actualSize,
                                                  const uint32_t offset,
                                                  const bool isTemporary,
                                                  ID3D12Resource *uploadBuffer,
                                                  ID3D12Resource *buffer) {
  assert(offset == 0);
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
  dx12::CURRENT_FRAME_RESOURCE->fc.commandList->ResourceBarrier(1,
                                                                &preTransition);
  UpdateSubresources<1>(commandList, buffer, uploadBuffer, 0, 0, 1,
                        &subResourceData);
  auto postTransition = CD3DX12_RESOURCE_BARRIER::Transition(
      buffer, D3D12_RESOURCE_STATE_COPY_DEST,
      D3D12_RESOURCE_STATE_GENERIC_READ);
  commandList->ResourceBarrier(1, &postTransition);

  // if it is temporary it means we are using this as intermediate upload
  // heap, we will free it when the upload is done, as such we will be
  // keeping track of the upload status with a fence
  if (isTemporary) {
    // Note: uploadBuffer has to be kept alive after the above function
    // calls because the command list has not been executed yet that
    // performs the actual copy. The caller can Release the uploadBuffer
    // after it knows the copy has been executed.
    m_uploadRequests.pushBack(
        UploadRequest{uploadBuffer, dx12::insertFenceToGlobalQueue()});
  }
}

// TODO needs some clean up, how come all the checks on the sourceState is not
// used?
void BufferManagerDx12::transitionBuffer(const BufferHandle handle,
                                         const BufferTransition &transition) {
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const BufferData &data = m_bufferPool.getConstRef(index);
  D3D12_RESOURCE_BARRIER barrier{};

  if (transition.sourceState == BUFFER_STATE_WRITE &&
      transition.destinationState == BUFFER_STATE_WRITE &&
      transition.sourceStage == BUFFER_STAGE_COMPUTE &&
      transition.destinationStage == BUFFER_STAGE_COMPUTE) {
    barrier.UAV.pResource = data.data;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    auto commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;
    commandList->ResourceBarrier(1, &barrier);
    return;
  }

  D3D12_RESOURCE_STATES sourceState;
  if (transition.sourceState == BUFFER_STATE_READ &&
      transition.sourceStage == BUFFER_STAGE_GRAPHICS) {
    sourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                  D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  }
  if (transition.sourceState == BUFFER_STATE_READ &&
      transition.sourceStage == BUFFER_STAGE_COMPUTE) {
    sourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  }
  if (transition.sourceState == BUFFER_STATE_WRITE) {
    sourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  }

  if (transition.destinationState == BUFFER_STATE_INDIRECT_DRAW) {
    assert(transition.sourceState != D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        data.data, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
  } else {
    barrier.UAV.pResource = data.data;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  }
  auto commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;
  commandList->ResourceBarrier(1, &barrier);
}

BufferHandle BufferManagerDx12::allocate(const uint32_t sizeInBytes,
                                         void *initData, const char *name,
                                         const int numElements,
                                         const int elementSize,
                                         const BUFFER_FLAGS flags) {
  // must be at least 256 bytes
  uint32_t actualSize =
      sizeInBytes % 256 == 0 ? sizeInBytes : ((sizeInBytes / 256) + 1) * 256;

  const bool isUav = (flags & BUFFER_FLAGS_BITS::RANDOM_WRITE) > 0;
  const bool isGpuOnly = (flags & BUFFER_FLAGS_BITS::GPU_ONLY) > 0;
  const bool isStatic = (flags & BUFFER_FLAGS_BITS::IS_STATIC) > 0;

  bool isTemporary = isGpuOnly & isStatic;
  // TODO if is GPU only we are leaking this buffer :D
  ID3D12Resource *uploadBuffer = allocateCpuVisibleBuffer(actualSize);

  ID3D12Resource *buffer = nullptr;
  void *uploadMappedData = nullptr;

  if (isGpuOnly) {
    buffer = allocateGpuVisibleBuffer(actualSize, isUav);
    buffer->SetName(persistentConvertWide(name));
  } else {
    HRESULT mapResult = uploadBuffer->Map(0, nullptr, &uploadMappedData);
    assert(SUCCEEDED(mapResult));
  }

  // if there is init data we need to take care of that
  if (initData != nullptr) {
    if (isGpuOnly) {
      uploadDataToGpuOnlyBuffer(initData, actualSize, 0, isTemporary,
                                uploadBuffer, buffer);
    } else {
      // we can do a memcpy directly since the buffer is cpu visible
      memcpy(uploadMappedData, initData, sizeInBytes);
    }
  }

  // lets get a data from the pool
  uint32_t index;
  BufferData &data = m_bufferPool.getFreeMemoryData(index);
  data = {};
  // if the buffer is not temporary we need to use the upload buffer
  data.data = isGpuOnly ? buffer : uploadBuffer;
  data.uploadBuffer = uploadBuffer;

  // data is now loaded need to create handle etc
  BufferHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.state = isUav ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                     : D3D12_RESOURCE_STATE_GENERIC_READ;
  data.elementCount = numElements;
  data.elementSize = elementSize;
  data.mappedData = uploadMappedData;
  data.flags = flags;

  return handle;
}

void BufferManagerDx12::createUav(const BufferHandle &buffer,
                                  DescriptorPair &descriptor, const int offset,
                                  const bool descriptorExits) {
  assertMagicNumber(buffer);
  const uint32_t index = getIndexFromHandle(buffer);
  const BufferData &data = m_bufferPool.getConstRef(index);

  // we need to check what the offset in elements is
  uint32_t elementOffset = offset / data.elementSize;

  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferUAV(
      descriptor, data.data, data.elementCount, data.elementSize, elementOffset,
      descriptorExits);
}

void BufferManagerDx12::createSrv(const BufferHandle &handle,
                                  DescriptorPair &descriptorPair,
                                  const uint32_t offset,
                                  const bool descriptorExits) const {
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const BufferData &data = m_bufferPool.getConstRef(index);

  // we need to check what the offset in elements is
  uint32_t elementOffset = offset / data.elementSize;

  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferSRV(
      descriptorPair, data.data, data.elementCount, data.elementSize,
      elementOffset, descriptorExits);
}
void BufferManagerDx12::createSrv(const BufferHandle &handle,
                                  DescriptorPair &descriptorPair,
                                  const MemoryRange range,
                                  const bool descriptorExists,
                                  const int elementSize) const {
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const BufferData &data = m_bufferPool.getConstRef(index);

  // the element size of the buffer might not be representative, for example
  // we might have a single buffer for the whole mesh, but we will make
  // different SRV out of it, for float4, float2 and so on, so we pass the
  // elementSize from outside if needed, if none is provided (-1) then we simply
  // use the buffer one
  int elemSize = elementSize == -1 ? data.elementSize : elementSize;
  int elementOffset = (range.m_offset / elemSize);
  int elementCount = range.m_size / elemSize;

  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferSRV(
      descriptorPair, data.data, elementCount, elemSize, elementOffset,
      descriptorExists);
}

void *BufferManagerDx12::getMappedData(const BufferHandle handle) const {
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const BufferData &data = m_bufferPool.getConstRef(index);
  assert((data.flags & BUFFER_FLAGS_BITS::GPU_ONLY) == 0);
  return data.mappedData;
}

void BufferManagerDx12::clearUploadRequests() {
  const auto id = GLOBAL_FENCE->GetCompletedValue();
  const int requestSize = static_cast<int>(m_uploadRequests.size()) - 1;
  int stackTopIdx = requestSize;
  for (int i = requestSize; i >= 0; --i) {
    UploadRequest &upload = m_uploadRequests[i];
    if (upload.fence < id) {
      // we can free the memory
      upload.uploadBuffer->Release();
      SE_CORE_INFO("Freed buffer upload with fence {0}", upload.fence);
      if (stackTopIdx != i) {
        // lets copy
        m_uploadRequests[i] = m_uploadRequests[stackTopIdx];
      }
      --stackTopIdx;
    }
  }
  // resizing the vector
  m_uploadRequests.resize(stackTopIdx + 1);
}

void BufferManagerDx12::update(const BufferHandle handle, void *inData,
                               int offset, int size) {
  const uint32_t index = getIndexFromHandle(handle);
  const BufferData &data = m_bufferPool.getConstRef(index);
  const bool isGpuOnly = (data.flags & BUFFER_FLAGS_BITS::GPU_ONLY) > 0;
  const bool isStatic = (data.flags & BUFFER_FLAGS_BITS::IS_STATIC) > 0;
  assert(data.uploadBuffer != nullptr);
  assert(isStatic == false);

  // if is gpu only we need to perform the buffer copy
  if (isGpuOnly) {
    uploadDataToGpuOnlyBuffer(inData, size, offset, false, data.uploadBuffer,
                              data.data);
  }
}
}  // namespace SirEngine::dx12
