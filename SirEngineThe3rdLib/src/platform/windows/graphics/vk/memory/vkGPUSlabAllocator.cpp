#include "platform/windows/graphics/vk/memory/vkGPUSlabAllocator.h"

#include "SirEngine/log.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkBufferManager.h"

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
    char* ptr =
        (char*)vk::BUFFER_MANAGER->getMappedData(m_slabs[freeSlab]->handle);
    /*
    memcpy(
        static_cast<char*>(m_slabs[freeSlab]->m_buffer.data) + range.m_offset,
        initialData, sizeInBytes);
        */
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
    //vkDestroyBuffer(vk::LOGICAL_DEVICE, m_slabs[i]->m_buffer.buffer, nullptr);
    //vkFreeMemory(vk::LOGICAL_DEVICE, m_slabs[i]->m_buffer.memory, nullptr);
      globals::BUFFER_MANAGER->free(m_slabs[i]->handle);
  }
}

void createBufferTemp(Buffer& buffer, const VkDevice device, const size_t size,
                      const VkBufferUsageFlags usage,
                      const VkMemoryPropertyFlags memoryFlags,
                      const char* name) {
  VkBufferCreateInfo createInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  createInfo.size = size;
  createInfo.usage = usage;
  createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // this is just a dummy handle
  VK_CHECK(vkCreateBuffer(device, &createInfo, nullptr, &buffer.buffer));
  SET_DEBUG_NAME(buffer.buffer, VK_OBJECT_TYPE_BUFFER,
                 frameConcatenation(name, "Buffer"));

  // memory bits of this struct define the requirement of the type of memory.
  // AMD seems to have 256 mb of mapped system memory you can write directly to
  // it. might be good for constant buffers changing often?
  // memory requirement type bits, is going to tell us which type of memory are
  // compatible with our buffer, meaning each one of them could host the
  // allocation
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(device, buffer.buffer, &requirements);
  buffer.size = size;
  buffer.allocationSize = requirements.size;

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(vk::PHYSICAL_DEVICE, &memoryProperties);
  // so, HOST_VISIBLE, means we can map this buffer to host memory, this is
  // useful for when we upload memory the gpu. similar to upload_heap in dx12.
  // Now in reality we should then copy this memory from gpu memory mapped
  // memory to DEVICE_BIT only, in this way it will be the fastest possible
  // access
  uint32_t memoryIndex = selectMemoryType(
      memoryProperties, requirements.memoryTypeBits, memoryFlags);

  if (memoryIndex == ~0u) {
    SE_CORE_WARN(
        "requested memory flags for constant buffer are not available, failing "
        "back to HOST_VISIBLE and HOST_CHOERENT, might have higher latency");
    uint32_t newMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    memoryIndex = selectMemoryType(memoryProperties,
                                   requirements.memoryTypeBits, newMemoryFlags);
    assert(memoryIndex != ~0u);
  }

  VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memoryInfo.allocationSize = requirements.size;
  memoryInfo.memoryTypeIndex = memoryIndex;

  VK_CHECK(vkAllocateMemory(device, &memoryInfo, nullptr, &buffer.memory));
  SET_DEBUG_NAME(buffer.memory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                 frameConcatenation(name, "Memory"));

  // binding the memory to our buffer, the dummy handle we allocated previously
  vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0);

  // now we map memory so we get a pointer we can write to
  VK_CHECK(vkMapMemory(device, buffer.memory, 0, buffer.size, 0, &buffer.data));
}

uint32_t VKGPUSlabAllocator::allocateSlab() {
  Slab* slab = new Slab(m_config.slabSizeInBytes);
  m_slabs.pushBack(slab);

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(vk::PHYSICAL_DEVICE, &memoryProperties);

  // We are requesting for memory that is device local BUT  host visible
  // when mapped it will give us a GPU pointer, we can write to directly
  // using memcpy, DO NOT DEREFERENCE ANY OF THE MEMORY!!!
  VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  vk::Buffer& buffer = slab->m_buffer;
  char printbuff[64];
  sprintf(printbuff, "gpuSlab%i", m_slabs.size() - 1);

  BufferHandle handle = vk::BUFFER_MANAGER->allocate(
      m_config.slabSizeInBytes, nullptr, printbuff, 10, sizeof(float),
      BufferManager::BUFFER_FLAGS::STORAGE_BUFFER 
      //|BufferManager::BUFFER_FLAGS::UPDATED_EVERY_FRAME
  );
  // createBufferTemp(buffer, vk::LOGICAL_DEVICE, m_config.slabSizeInBytes,
  //                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, memoryFlags,
  //                 printbuff);
  slab->handle = handle;

  return m_slabs.size() - 1;
}

// as of now memory is allocated in so called slabs, such slabs are nothing more
// than a range of memory to be filled up, when the memory is full, a new slab
// gets allocated keep in mind memory is not meant to be de-allocated right now
// is a simple implementation:. as such also slabs are automatically buffered,
// when you allocating one, in reality you allocating N of them where N is the
// number of frame in flight.
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
