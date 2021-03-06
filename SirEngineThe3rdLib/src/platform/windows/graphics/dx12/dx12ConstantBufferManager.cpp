#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"

#include "d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"

namespace SirEngine::dx12 {

void Dx12ConstantBufferManager::initialize() {
  m_randomAlloc.initialize(4096, 20);
}

void Dx12ConstantBufferManager::cleanup() {
  // TODO do proper cleanup
}

void Dx12ConstantBufferManager::clearUpQueueFree() {
  const uint64_t id = GLOBAL_FENCE->GetCompletedValue();
  const auto count = static_cast<int>(m_bufferToFree.size()) - 1;
  int stackTopIdx = count;

  for (int i = count; i >= 0; --i) {

    const ConstantBufferHandle handle{m_bufferToFree[i]};
    const int32_t index = getIndexFromHandle(handle);
    ConstantBufferDataDynamic &data = m_dynamicStorage[index];

    for (int c = 0; c < FRAME_BUFFERS_COUNT; ++c) {
      if (data.cbData[c].fence < id) {
        // we need to remove it from the request list if it is there
        auto found = m_bufferedRequests.find(handle.handle);
        if (found != m_bufferedRequests.end()) {
          // freeing the random allocation
          m_randomAlloc.freeAllocation(found->second.dataAllocHandle);
          // removing the actuall entry
          m_bufferedRequests.erase(found);
        }

        dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(data.cbData[c].pair);
        unmapConstantBuffer(data.cbData[c]);

        // we can free the memory
        data.cbData[c].resource->Release();
        if (stackTopIdx != i) {
          // lets copy
          m_bufferToFree[i] = m_bufferToFree[stackTopIdx];
          --stackTopIdx;
        }
      }
    }
  }
}

void Dx12ConstantBufferManager::createSrv(
    const ConstantBufferHandle &bufferHandle, DescriptorPair &descriptorPair) {
  assertMagicNumber(bufferHandle);
  uint32_t index = getIndexFromHandle(bufferHandle);
  auto data = m_dynamicStorage[index].cbData[globals::CURRENT_FRAME];

  const uint32_t actualSize =
      data.size % 256 == 0 ? data.size : ((data.size / 256) + 1) * 256;
  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferCBV(descriptorPair, data.resource,
                                                 actualSize, true);
}

inline bool isFlagSet(
    const uint32_t flags,
    const ConstantBufferManager::CONSTANT_BUFFER_FLAGS toCheck) {
  return (flags & toCheck) > 0;
}

void Dx12ConstantBufferManager::update(const ConstantBufferHandle handle,
                                       void *inputData) {
  // check if we have any other request for this buffer if so we clear it
  const auto found = m_bufferedRequests.find(handle.handle);
  if (found != m_bufferedRequests.end()) {
    // lets clear up the allocation
    m_randomAlloc.freeAllocation(found->second.dataAllocHandle);
  }

  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const ConstantBufferData data =
      m_dynamicStorage[index].cbData[globals::CURRENT_FRAME];

  bool updatedEveryFrame = isFlagSet(data.flags, UPDATED_EVERY_FRAME);
  // if we update every frame like the camera buffer for example let us just
  // skip extra steps
  if (updatedEveryFrame) {
    updateConstantBufferNotBuffered(handle, inputData);
    return;
  }

  // lets create a new request
  ConstantBufferedData buffRequest;
  buffRequest.poolIndex = index;

  // setting data on the buffer request
  buffRequest.dataAllocHandle = m_randomAlloc.allocate(data.size);
  buffRequest.handle = handle;

  // perform current update, that is why we use frame buffer count -1
  buffRequest.counter = FRAME_BUFFERS_COUNT - 1;
  updateConstantBufferNotBuffered(handle, inputData);
  // copying data in storage so we can keep track of it
  memcpy(m_randomAlloc.getPointer(buffRequest.dataAllocHandle), inputData,
         data.size);
  m_bufferedRequests[handle.handle] = buffRequest;
}

bool Dx12ConstantBufferManager::free(ConstantBufferHandle handle) {
  // here we insert a fence so that we know when will be safe to delete
  // making sure the resource has not been de-allocated
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const uint64_t fence = dx12::insertFenceToGlobalQueue();
  for (int i = 0; i < FRAME_BUFFERS_COUNT; ++i) {
    m_dynamicStorage[index].cbData[i].fence = fence;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(m_dynamicStorage[index].cbData[i].pair);
  }
  m_bufferToFree.push_back(handle.handle);
  // now we can set the index ready to be freed in the queue

  return false;
}

ConstantBufferHandle Dx12ConstantBufferManager::allocate(
    const uint32_t sizeInBytes, const CONSTANT_BUFFER_FLAGS flags,
    void *inputData) {
  // everything for now is allocated dynamically for sake of simplicity, takes
  // into account we could have multiple frame in flight and we don not want to
  // override the value meanwhile is still in use, so multiple version are
  // allocated same number of flame inflight the engine is using

  // must be at least 256 bytes, size is clamped if too small
  const uint32_t actualSize =
      sizeInBytes % 256 == 0 ? sizeInBytes : ((sizeInBytes / 256) + 1) * 256;

  // finding a free block in the pool
  uint32_t index = static_cast<uint32_t>(-1);
  ConstantBufferDataDynamic &data = m_dynamicStorage.getFreeMemoryData(index);
  const ConstantBufferHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

  // here we allocate the N constant buffers
  for (int i = 0; i < FRAME_BUFFERS_COUNT; ++i) {
    // how does this work? well vulkan works with slabs, so constant buffers are
    // sub allocated from a bigger chunk (slab) , and you have multiple slab in
    // flight. Here instead we slice the problem differently and each allocation
    // has 3 smaller resources. I like the Vulkan way better and will be changed
    // in the future
    data.cbData[i].mapped = false;
    data.cbData[i].size = sizeInBytes;

    // create the upload buffer
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(actualSize);

    HRESULT result = dx12::DEVICE->CreateCommittedResource(
        &heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&data.cbData[i].resource));
    assert(SUCCEEDED(result));

    // allocating a descriptor
    DescriptorPair pair{};
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferCBV(
        pair, data.cbData[i].resource, actualSize);
    // map the buffer and copy the memory over
    mapConstantBuffer(data.cbData[i]);
    if (inputData != nullptr) {
      memcpy(data.cbData[i].mappedData, inputData, sizeInBytes);
    }

    data.cbData[i].pair = pair;
    data.cbData[i].magicNumber = MAGIC_NUMBER_COUNTER;
    data.cbData[i].flags = flags;
    assert(data.cbData[i].resource != nullptr);
  }
  ++MAGIC_NUMBER_COUNTER;
  return handle;
}

void Dx12ConstantBufferManager::updateConstantBufferNotBuffered(
    const ConstantBufferHandle handle, void *dataToUpload) {
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const ConstantBufferData &data =
      m_dynamicStorage[index].cbData[globals::CURRENT_FRAME];

  assert(data.mappedData != nullptr);
  memcpy(data.mappedData, dataToUpload, data.size);
}

void Dx12ConstantBufferManager::processBufferedData() {
  std::vector<int> processedIdxs;
  const int bufferedRequests = static_cast<int>(m_bufferedRequests.size());
  processedIdxs.reserve(bufferedRequests);

  for (auto &handle : m_bufferedRequests) {
    char *ptr = m_randomAlloc.getPointer(handle.second.dataAllocHandle);
    updateConstantBufferNotBuffered(handle.second.handle, ptr);
    handle.second.counter -= 1;
    if (handle.second.counter == 0) {
      processedIdxs.push_back(handle.first);
    }
  }

  // cleanup
  const int toDeleteCount = static_cast<int>(processedIdxs.size());
  for (int i = 0; i < toDeleteCount; ++i) {
    const ConstantBufferedData &data = m_bufferedRequests[processedIdxs[i]];
    m_randomAlloc.freeAllocation(data.dataAllocHandle);
    m_bufferedRequests.erase(m_bufferedRequests.find(processedIdxs[i]));
  }
}
}  // namespace SirEngine::dx12
