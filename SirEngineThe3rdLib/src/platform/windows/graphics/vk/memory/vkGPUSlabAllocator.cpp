#include "platform/windows/graphics/vk/memory/vkGPUSlabAllocator.h"

#include "SirEngine/bufferManager.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/vk/vk.h"

namespace SirEngine::vk {
void VKGPUSlabAllocator::initialize(
    const GPUSlabAllocatorInitializeConfig config) {
  m_config = config;
  uint32_t allocCount = m_config.initialSlabs == 0 ? 1 : m_config.initialSlabs;
  for (uint32_t i = 0; i < allocCount; ++i) {
    allocateSlab();
  }

  // checking alignment requirements
  // VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(vk::PHYSICAL_DEVICE, &properties);
  m_requireAlignment = properties.limits.minStorageBufferOffsetAlignment;
}

GPUSlabAllocationHandle VKGPUSlabAllocator::allocate(const uint32_t sizeInBytes,
                                                     void* initialData) {
  // search the slabs, slabs per frame are exactly identical, searching one will
  // yield the same result of searching all of them
  const int freeSlab = getFreeSlabIndex(sizeInBytes);

  BufferRangeHandle handle{};
  BufferRange range{};
  handle = m_slabs[freeSlab]->m_slabTracker.allocate(sizeInBytes,
                                                     m_requireAlignment);
  range = m_slabs[freeSlab]->m_slabTracker.getBufferRange(handle);
  // copying the data if we have a valid pointer
  if (initialData != nullptr) {
    // offsetting the pointer by the amount the tracker tells us
    assert(sizeInBytes <= range.m_size);
    char* ptr = static_cast<char*>(globals::BUFFER_MANAGER->getMappedData(
	    m_slabs[freeSlab]->handle));

    //TODO probably we want to do some checks on whether or not is doable
    memcpy(ptr + range.m_offset, initialData, sizeInBytes);
  }

  // if we are here we allocated successfully!
  // the handle is composed of the slab index and buffer range handle
  // finding a free block in the pool
  uint32_t index;
  SlabAllocData& buffData = m_allocInfoStorage.getFreeMemoryData(index);
  buffData.m_version = VERSION_COUNTER++;
  buffData.m_rangeHandle = handle;
  buffData.m_range = range;
  buffData.m_slabIdx = freeSlab;

  const GPUSlabAllocationHandle outHandle{(buffData.m_version << 16) | index};
  return outHandle;
}

void VKGPUSlabAllocator::clear() {
  // assert(0);
  for (int i = 0; i < m_slabs.size(); ++i) {
    m_slabs[i]->m_slabTracker.clear();
  }
  m_allocInfoStorage.clear();
}

void* VKGPUSlabAllocator::getMappedPtr(GPUSlabAllocationHandle allocHandle) {
  return nullptr;
}

void VKGPUSlabAllocator::cleanup() {
  int count = m_slabs.size();
  for (int i = 0; i < count; ++i) {
    globals::BUFFER_MANAGER->free(m_slabs[i]->handle);
  }
}

uint32_t VKGPUSlabAllocator::allocateSlab() {
  Slab* slab = new Slab(m_config.slabSizeInBytes);
  m_slabs.pushBack(slab);

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(vk::PHYSICAL_DEVICE, &memoryProperties);

  char printbuff[64];
  sprintf(printbuff, "gpuSlab%i", m_slabs.size() - 1);

  BufferHandle handle = globals::BUFFER_MANAGER->allocate(
      m_config.slabSizeInBytes, nullptr, printbuff, 10, sizeof(float),
      BufferManager::BUFFER_FLAGS::STORAGE_BUFFER
      //|BufferManager::BUFFER_FLAGS::UPDATED_EVERY_FRAME
  );
  slab->handle = handle;
  return m_slabs.size() - 1;
}

int VKGPUSlabAllocator::getFreeSlabIndex(const uint32_t allocSize) {
  int freeSlab = -1;
  for (uint32_t i = 0; i < m_slabs.size(); ++i) {
    bool canAllocate = m_slabs[i]->m_slabTracker.canAllocate(allocSize);
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
  return allocateSlab();
}

}  // namespace SirEngine::vk
