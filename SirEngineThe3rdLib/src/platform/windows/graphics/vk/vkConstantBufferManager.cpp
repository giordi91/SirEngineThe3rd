#include "platform/windows/graphics/vk/vkConstantBufferManager.h"
#include "platform/windows/graphics/vk/vk.h"
#include "vkMemory.h"
#include <random>

namespace SirEngine::vk {

void VkConstantBufferManager::initialize() {
  m_randomAlloc.initialize(4096, 20);

  // allocate the memory used for all the slabs
  uint32_t totalMemory =
      sizeof(Slab) * MAX_ALLOCATED_SLABS * vk::SWAP_CHAIN_IMAGE_COUNT;
  m_slabs = reinterpret_cast<Slab *>(
      globals::PERSISTENT_ALLOCATOR->allocate(totalMemory));

  m_perFrameSlabs =
      reinterpret_cast<Slab **>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(Slab **) * vk::SWAP_CHAIN_IMAGE_COUNT));

  memset(m_slabs, 0, totalMemory);
  memset(m_perFrameSlabs, 0, sizeof(Slab *) * vk::SWAP_CHAIN_IMAGE_COUNT);
  for (uint32_t i = 0; i < vk::SWAP_CHAIN_IMAGE_COUNT; ++i) {
    m_perFrameSlabs[i] = &m_slabs[i * MAX_ALLOCATED_SLABS];
  }

  // initialize at least one slab
  allocateSlab();
}

void VkConstantBufferManager::cleanup() {
  for (uint32_t i = 0; i < vk::SWAP_CHAIN_IMAGE_COUNT; ++i) {
    for (int pf = 0; pf < m_allocatedSlabs; ++pf) {
      Slab *slab = &m_perFrameSlabs[i][pf];
      destroyBuffer(vk::LOGICAL_DEVICE, slab->m_buffer);
      // created with placement new, need to call destructor directly,
      // but won't call delete, since memory is owned by the allocator
      m_perFrameSlabs[i][pf].~Slab();
    }
  }
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
inline bool
isFlagSet(const uint32_t flags,
          const ConstantBufferManager::CONSTANT_BUFFER_FLAGS toCheck) {
  return (flags & toCheck) > 0;
}

bool VkConstantBufferManager::free(const ConstantBufferHandle handle) {

  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  const ConstantBufferData &buffData = m_allocInfoStorage.getConstRef(idx);

  bool isBuffered = isFlagSet(buffData.m_flags,
                                             CONSTANT_BUFFER_FLAGS::BUFFERED);
  int slabIdx = buffData.m_slabIdx;
  assert(isBuffered);
  // first let us free the tracker
  for (int i = 0; i < vk::SWAP_CHAIN_IMAGE_COUNT; ++i) {
    m_perFrameSlabs[i][slabIdx].m_slabTracker.free(buffData.m_rangeHandle);
  }
  // next we just need to free the pool
  m_allocInfoStorage.free(idx);

  return false;
}

ConstantBufferHandle
VkConstantBufferManager::allocateDynamic(const uint32_t sizeInBytes,
                                         void *inputData) {

  return {};
}

inline uint32_t padTo32BytesMultiple(const uint32_t size) {
  return size % 32 != 0 ? (size / 32 + 1) * 32 : size;
}

int VkConstantBufferManager::getFreeSlabIndex(uint32_t allocSize) {

  int freeSlab = -1;
  for (uint32_t i = 0; i < m_allocatedSlabs; ++i) {
    bool canAllocate =
        m_perFrameSlabs[0][i].m_slabTracker.canAllocate(allocSize);
    if (!canAllocate) {
      continue;
    }
    freeSlab = i;
    break;
  }
  if (freeSlab != -1) {
    // we found a slab, good to go
    return freeSlab;
  }
  // no slab found but can we allocate?
  if (m_allocatedSlabs < MAX_ALLOCATED_SLABS) {
    int toReturnFreeSlabIdx = m_allocatedSlabs;
    allocateSlab();
    return toReturnFreeSlabIdx;
  }

  // no free slab and no chance to allocate more
  assert(0 && "cannot allocate any more slab for uniform data");
  return -1;
}
ConstantBufferHandle
VkConstantBufferManager::allocBuffered(const uint32_t sizeInBytes,
                                       const uint32_t flags, void *data) {
  uint32_t allocSize = padTo32BytesMultiple(sizeInBytes);

  // search the slabs, slabs per frame are exactly identical, searching one will
  // yield the same result of searching all of them
  int freeSlab = getFreeSlabIndex(allocSize);

#ifdef SE_DEBUG
  // we want to make sure all the handles are the same, since we want to keep
  // the slabs in sync
  auto *handles =
      reinterpret_cast<BufferRangeHandle *>(globals::FRAME_ALLOCATOR->allocate(
          sizeof(BufferRangeHandle) * vk::SWAP_CHAIN_IMAGE_COUNT));
#endif
  // we have a valid range, lets loop the per frame slabs (aka pfs)
  BufferRangeHandle handle{};
  BufferRange range{};
  for (uint32_t pfs = 0; pfs < vk::SWAP_CHAIN_IMAGE_COUNT; ++pfs) {
    handle = m_perFrameSlabs[pfs][freeSlab].m_slabTracker.allocate(sizeInBytes);
    range = m_perFrameSlabs[pfs][freeSlab].m_slabTracker.getBufferRange(handle);
    // copying the data if we have a valid pointer
    if (data != nullptr) {
      // offsetting the pointer by the amount the tracker tells us
      assert(sizeInBytes <= range.m_size);
      memcpy(static_cast<char *>(m_perFrameSlabs[pfs][freeSlab].m_buffer.data) +
                 range.m_offset,
             data, sizeInBytes);
    }
#ifdef SE_DEBUG
    handles[pfs] = handle;
#endif
  }

#ifdef SE_DEBUG
  // let us know check if all the andles are the same
  for (uint32_t pf = 1; pf < vk::SWAP_CHAIN_IMAGE_COUNT; ++pf) {
    assert(handles[0].handle == handles[freeSlab].handle);
  }
#endif

  // iff we are here we allocated successfully!
  // the handle is composed of the slab index and buffer range handle
  // finding a free block in the pool
  uint32_t index;
  ConstantBufferData &buffData = m_allocInfoStorage.getFreeMemoryData(index);
  buffData.m_magicNumber = MAGIC_NUMBER_COUNTER++;
  buffData.m_flags = flags;
  buffData.m_rangeHandle = handle;
  buffData.m_range = range;
  buffData.m_slabIdx = freeSlab;

  const ConstantBufferHandle outHandle{(buffData.m_magicNumber << 16) | index};
  return outHandle;
}

SirEngine::ConstantBufferHandle
VkConstantBufferManager::allocate(const uint32_t sizeInBytes,
                                  const uint32_t flags, void *data) {

  bool isBuffered =
      SirEngine::vk::isFlagSet(flags, CONSTANT_BUFFER_FLAGS::BUFFERED);
  // not yet implemented not buffered data, aka static data
  assert(isBuffered == true);
  return allocBuffered(sizeInBytes, flags, data);
}

void VkConstantBufferManager::update(const ConstantBufferHandle handle,
                                     void *data) {
  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  const ConstantBufferData &buffData = m_allocInfoStorage.getConstRef(idx);

  bool isBuffered = isFlagSet(buffData.m_flags, BUFFERED);
  // for now supporting only buffered
  assert(isBuffered == true);

  bool updatedEveryFrame = isFlagSet(buffData.m_flags, UPDATED_EVERY_FRAME);

  // if we override it every frame, there is no point in doing the whole
  // buffered thing which a lot more expensive
  if (updatedEveryFrame) {
    int slabIndex = buffData.m_slabIdx;
    const Slab &slab = m_perFrameSlabs[globals::CURRENT_FRAME][slabIndex];
    assert(data != nullptr);
    memcpy(static_cast<char *>(slab.m_buffer.data) + buffData.m_range.m_offset,
           data, buffData.m_range.m_size);
    return;
  }

  bool submitBufferedRequest = !updatedEveryFrame && isBuffered;
  if (submitBufferedRequest) {
    // we need to do the proper
    assert(0);
  }
}

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

void VkConstantBufferManager::bindConstantBuffer(
    const ConstantBufferHandle handle, VkDescriptorBufferInfo &bufferInfo,
    const uint32_t bindingIdx, VkWriteDescriptorSet *set,
    const VkDescriptorSet descSet) {

  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  const ConstantBufferData &buffData = m_allocInfoStorage.getConstRef(idx);
  const Slab &slab =
      m_perFrameSlabs[globals::CURRENT_FRAME][buffData.m_slabIdx];

  // actual information of the descriptor, in this case it is our mesh buffer
  bufferInfo.buffer = slab.m_buffer.buffer;
  bufferInfo.offset = buffData.m_range.m_offset;
  bufferInfo.range = buffData.m_range.m_size;

  VkWriteDescriptorSet &correctSet = set[bindingIdx];

  correctSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  correctSet.dstSet = descSet;
  correctSet.dstBinding = 1;
  correctSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  correctSet.pBufferInfo = &bufferInfo;
  correctSet.descriptorCount = 1;
}

const ResizableVector<BufferRangeTracker> *
VkConstantBufferManager::getAllocations() const {
  return m_perFrameSlabs[0]->m_slabTracker.getAllocations();
}

void VkConstantBufferManager::allocateSlab() {

  assert(m_allocatedSlabs < MAX_ALLOCATED_SLABS);
  int newSlabIdx = m_allocatedSlabs++;
  for (uint32_t i = 0; i < vk::SWAP_CHAIN_IMAGE_COUNT; ++i) {

    // m_slabs[i * MAX_ALLOCATED_SLABS + newSlabIdx] = new Slab();
    void *allocPtr = &m_slabs[i * MAX_ALLOCATED_SLABS + newSlabIdx];
    auto *slab = new (allocPtr) Slab();

    // Slab *slab = m_slabs[newSlabIdx];

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vk::PHYSICAL_DEVICE, &memoryProperties);

    // We are requesting for memory that is device local BUT  host visible
    // when mapped it will give us a GPU pointer, we can write to directly
    // using memcpy, DO NOT DEREFERENCE ANY OF THE MEMORY!!!
    VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vk::Buffer &buffer = slab->m_buffer;

    char counterChar[2];
    counterChar[0] = static_cast<char>(m_allocatedSlabs + 48);
    counterChar[1] = '\0';

    uint64_t slabSize = SLAB_ALLOCATION_IN_MB * MB_TO_BYTE;

    createBuffer(buffer, vk::LOGICAL_DEVICE, slabSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memoryFlags,
                 frameConcatenation("slab", counterChar));
  }
}
} // namespace SirEngine::vk