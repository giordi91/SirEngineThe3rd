#include "platform/windows/graphics/vk/vkConstantBufferManager.h"
#include "platform/windows/graphics/vk/vk.h"
#include "vkMemory.h"

namespace SirEngine::vk {

void VkConstantBufferManager::initialize() {
  m_randomAlloc.initialize(4096, 20);

  // initialize at least one slab
}

void VkConstantBufferManager::clearUpQueueFree() {

  /*
  const uint64_t id = GLOBAL_FENCE->GetCompletedValue();
  const auto count = static_cast<int>(m_bufferToFree.size()) - 1;
  int stackTopIdx = count;

  for (int i = count; i >= 0; --i) {
    const ConstantBufferHandle handle{m_bufferToFree[i]};
    const int32_t index = getIndexFromHandle(handle);
    ConstantBufferDataDynamic &data = m_dynamicStorage[index];

    for (int c = 0; c < FRAME_BUFFERS_COUNT; ++c) {

      if (data.cbData[i].fence < id) {

        // we need to remove it from the request list if it is there
        auto found = m_bufferedRequests.find(handle.handle);
        if (found != m_bufferedRequests.end()) {
          // freeing the random allocation
          m_randomAlloc.freeAllocation(found->second.dataAllocHandle);
          // removing the actuall entry
          m_bufferedRequests.erase(found);
        }

        // we can free the memory
        data.cbData[i].resource->Release();

        dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(data.cbData[i].pair);
        unmapConstantBuffer(data.cbData[i]);
        if (stackTopIdx != i) {
          // lets copy
          m_bufferToFree[i] = m_bufferToFree[stackTopIdx];
        }
        --stackTopIdx;
      }
    }
  }
  */
}

bool VkConstantBufferManager::free(const ConstantBufferHandle handle) {
    /*
  // here we insert a fence so that we know when will be safe to delete
  // making sure the resource has not been de-allocated
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const uint64_t fence = dx12::insertFenceToGlobalQueue();
  for (int i = 0; i < FRAME_BUFFERS_COUNT; ++i) {
    m_dynamicStorage[index].cbData[i].fence = fence;
  }
  m_bufferToFree.push_back(handle.handle);
  // now we can set the index ready to be freed in the queue

  */
  return false;
}

ConstantBufferHandle
VkConstantBufferManager::allocateDynamic(const uint32_t sizeInBytes,
                                         void *inputData) {

  /*
// allocate dynamics takes into account we could have multiple frame in
// flight and we don not want to override the value meanwhile is still in
// use, so multiple version are allocated same number of flame inflight the
// engine is using

// must be at least 256 bytes, size is clamped if too small
const uint32_t actualSize =
    sizeInBytes % 256 == 0 ? sizeInBytes : ((sizeInBytes / 256) + 1) * 256;

// finding a free block in the pool
uint32_t index;
ConstantBufferDataDynamic &data = m_dynamicStorage.getFreeMemoryData(index);

const ConstantBufferHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

// here we allocate the N constant buffers
for (int i = 0; i < FRAME_BUFFERS_COUNT; ++i) {
  data.cbData[i].mapped = false;
  data.cbData[i].size = sizeInBytes;

  // create the upload buffer
  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(actualSize);

  dx12::DEVICE->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&data.cbData[i].resource));

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
  // m_dynamicStorage[i].push_back(data);
  assert(data.cbData[i].resource != nullptr);
}
++MAGIC_NUMBER_COUNTER;
return handle;
*/
  return {};
}

ConstantBufferHandle VkConstantBufferManager::allocate(uint32_t sizeInBytes,
                                                       uint32_t flags,
                                                       void *data) { return {};}

void VkConstantBufferManager::updateConstantBufferNotBuffered(
    const ConstantBufferHandle handle, void *dataToUpload) {
  /*
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  const ConstantBufferData &data =
      m_dynamicStorage[index].cbData[globals::CURRENT_FRAME];

  assert(data.mappedData != nullptr);
  memcpy(data.mappedData, dataToUpload, data.size);
  */
}

void VkConstantBufferManager::updateConstantBufferBuffered(
    const ConstantBufferHandle handle, void *dataToUpload) {

  /*
  // check if we have any other request for this buffer if so we clear it
  const auto found = m_bufferedRequests.find(handle.handle);
  if (found != m_bufferedRequests.end()) {
    // lets clear up the allocation
    m_randomAlloc.freeAllocation(found->second.dataAllocHandle);
  }

  // lets create a new request
  ConstantBufferedData buffRequest;
  assertMagicNumber(handle);
  const uint32_t index = getIndexFromHandle(handle);
  buffRequest.poolIndex = index;

  const ConstantBufferData data =
      m_dynamicStorage[index].cbData[globals::CURRENT_FRAME];

  // setting data on the buffer request
  buffRequest.dataAllocHandle = m_randomAlloc.allocate(data.size);
  buffRequest.handle = handle;

  // perform current update, that is why we use frame buffer count -1
  buffRequest.counter = FRAME_BUFFERS_COUNT - 1;
  updateConstantBufferNotBuffered(handle, dataToUpload);
  // copying data in storage so we can keep track of it
  memcpy(m_randomAlloc.getPointer(buffRequest.dataAllocHandle), dataToUpload,
         data.size);
  m_bufferedRequests[handle.handle] = buffRequest;
  */
}

void VkConstantBufferManager::processBufferedData() {
  /*
  std::vector<int> processedIdxs;
  const int bufferedRequests = static_cast<int>(m_bufferedRequests.size());
  processedIdxs.reserve(bufferedRequests);

  for (auto &handle : m_bufferedRequests) {
    uchar *ptr = m_randomAlloc.getPointer(handle.second.dataAllocHandle);
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
  */
}

void VkConstantBufferManager::allocateSlab() {

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(vk::PHYSICAL_DEVICE, &memoryProperties);

  vk::Buffer buffer{};
  uint64_t slabSize = SLAB_ALLOCATION_IN_MB * MB_TO_BYTE;

  VkBufferCreateInfo createInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  createInfo.size = slabSize;
  createInfo.usage =
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // this is just a dummy handle
  char counterChar[2];
  counterChar[0] = static_cast<char>(m_allocatedSlabs + 48);
  counterChar[1] = '\0';
  VK_CHECK(vkCreateBuffer(vk::LOGICAL_DEVICE, &createInfo, nullptr, &buffer.buffer));
  SET_DEBUG_NAME(buffer.buffer, VK_OBJECT_TYPE_BUFFER,
                 frameConcatenation("uniformSlabBuffer", counterChar));

  // memory bits of this struct define the requirement of the type of memory.
  // AMD seems to have 256 mb of mapped system memory you can write directly to
  // it. might be good for constant buffers changing often?
  // memory requirement type bits, is going to tell us which type of memory are
  // compatible with our buffer, meaning each one of them could host the
  // allocation
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(vk::LOGICAL_DEVICE, buffer.buffer, &requirements);
  buffer.size = slabSize;
  buffer.allocationSize = requirements.size;

  // so, HOST_VISIBLE, means we can map this buffer to host memory, this is
  // useful for when we upload memory the gpu. similar to upload_heap in dx12.
  // Now in reality we should then copy this memory from gpu memory mapped
  // memory to DEVICE_BIT only, in this way it will be the fastest possible
  // access
  const uint32_t memoryIndex =
      selectMemoryType(memoryProperties, requirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  assert(memoryIndex != ~0u);

  VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memoryInfo.allocationSize = requirements.size;
  memoryInfo.memoryTypeIndex = memoryIndex;

  VK_CHECK(vkAllocateMemory(vk::LOGICAL_DEVICE, &memoryInfo, nullptr, &buffer.memory));
  SET_DEBUG_NAME(buffer.memory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                 frameConcatenation( "uniformSlabMemory",counterChar));

  // binding the memory to our buffer, the dummy handle we allocated previously
  vkBindBufferMemory(LOGICAL_DEVICE, buffer.buffer, buffer.memory, 0);

  // now we map memory so we get a pointer we can write to
  VK_CHECK(vkMapMemory(vk::LOGICAL_DEVICE, buffer.memory, 0, buffer.size, 0, &buffer.data));
}
} // namespace SirEngine::vk
