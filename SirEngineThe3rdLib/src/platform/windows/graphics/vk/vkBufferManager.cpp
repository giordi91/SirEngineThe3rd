#include "platform/windows/graphics/vk/vkBufferManager.h"

#include <random>

#include "platform/windows/graphics/vk/vk.h"
#include "vkMemory.h"

namespace SirEngine::vk {

void VkBufferManager::initialize() {
  m_randomAlloc.initialize(RANDOM_ALLOC_RESERVE);
}

void VkBufferManager::cleanup() {}

void VkBufferManager::free(const BufferHandle handle) {
  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  const Buffer &buffData = m_bufferStorage.getConstRef(idx);

  vkFreeMemory(vk::LOGICAL_DEVICE, buffData.memory, nullptr);
  vkDestroyBuffer(vk::LOGICAL_DEVICE, buffData.buffer, nullptr);
  // next we just need to free the pool
  m_bufferStorage.free(idx);
}

void *VkBufferManager::getMappedData(const BufferHandle handle) const {
  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  const Buffer &data = m_bufferStorage.getConstRef(idx);
  return data.memory;
}

void VkBufferManager::bindBuffer(BufferHandle handle,
                                 VkWriteDescriptorSet *write,
                                 VkDescriptorSet set, uint32_t bindingIndex) {
  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  const Buffer &data = m_bufferStorage.getConstRef(idx);

  write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write->dstSet = set;
  write->dstBinding = bindingIndex;
  //TODO this should be probably be queried by the buffer data
  write->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write->pBufferInfo = &data.info;
  write->descriptorCount = 1;
}

BufferHandle VkBufferManager::allocate(const uint32_t sizeInBytes,
                                       void *initData, const char *name,
                                       const int, const int,
                                       const uint32_t flags) {
  bool isRandomWrite = (flags & BUFFER_FLAGS::RANDOM_WRITE) > 0;
  bool isIndex = (flags & BUFFER_FLAGS::INDEX_BUFFER) > 0;
  bool isIndirectBuffer = (flags & BUFFER_FLAGS::INDIRECT_BUFFER) > 0;
  bool isVertexBuffer = (flags & BUFFER_FLAGS::VERTEX_BUFFER) > 0;
  bool isBuffered = (flags & BUFFER_FLAGS::BUFFERED) > 0;
  bool isUpdatedEveryFrame = (flags & BUFFER_FLAGS::UPDATED_EVERY_FRAME) > 0;
  bool isStorage = (flags & BUFFER_FLAGS::STORAGE_BUFFER) > 0;
  // better be safe than sorry, extensive checks on flags combinations
  assert(!isBuffered && "not supported yet");
  assert(!isUpdatedEveryFrame && "not supported yet");
  assert(!isIndirectBuffer && "not supported yet");
  assert(!(isVertexBuffer && isIndex) &&
         "canont be both vertex and index buffer");

  // process the flags
  uint32_t usage = isRandomWrite ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
  usage |= isIndex ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
  usage |= isIndirectBuffer ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0;
  usage |= isVertexBuffer | isStorage ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;

  VkBufferCreateInfo createInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  createInfo.size = sizeInBytes;
  createInfo.usage = usage;
  createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  uint32_t index = 0;
  vk::Buffer &buffer = m_bufferStorage.getFreeMemoryData(index);

  // this is just a dummy handle
  VK_CHECK(
      vkCreateBuffer(vk::LOGICAL_DEVICE, &createInfo, nullptr, &buffer.buffer));
  SET_DEBUG_NAME(buffer.buffer, VK_OBJECT_TYPE_BUFFER,
                 frameConcatenation(name, "Buffer"));

  // memory bits of this struct define the requirement of the type of memory.
  // AMD seems to have 256 mb of mapped system memory you can write directly to
  // it. might be good for constant buffers changing often?
  // memory requirement type bits, is going to tell us which type of memory are
  // compatible with our buffer, meaning each one of them could host the
  // allocation
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(vk::LOGICAL_DEVICE, buffer.buffer,
                                &requirements);
  buffer.size = sizeInBytes;
  buffer.allocationSize = requirements.size;

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(vk::PHYSICAL_DEVICE, &memoryProperties);

  // We are requesting for memory that is device local BUT  host visible
  // when mapped it will give us a GPU pointer, we can write to directly
  // using memcpy, DO NOT DEREFERENCE ANY OF THE MEMORY!!!
  VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  uint32_t memoryIndex = selectMemoryType(
      memoryProperties, requirements.memoryTypeBits, memoryFlags);

  if (memoryIndex == ~0u) {
    SE_CORE_WARN(
        "requested memory flags for buffers are not available, failing "
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

  VK_CHECK(vkAllocateMemory(vk::LOGICAL_DEVICE, &memoryInfo, nullptr,
                            &buffer.memory));
  SET_DEBUG_NAME(buffer.memory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                 frameConcatenation(name, "Memory"));

  // binding the memory to our buffer, the dummy handle we allocated previously
  vkBindBufferMemory(vk::LOGICAL_DEVICE, buffer.buffer, buffer.memory, 0);

  // now we map memory so we get a pointer we can write to
  VK_CHECK(vkMapMemory(vk::LOGICAL_DEVICE, buffer.memory, 0, buffer.size, 0,
                       &buffer.data));
  if (initData != nullptr) {
    memcpy(buffer.data, initData, sizeInBytes);
  }

  buffer.m_magicNumber = MAGIC_NUMBER_COUNTER++;
  buffer.info.buffer = buffer.buffer;
  buffer.info.offset = 0;
  buffer.info.range = buffer.allocationSize;
  // creating a handle
  BufferHandle handle{(buffer.m_magicNumber << 16) | index};
  return handle;
}

/*
void VkBufferManager::update(const ConstantBufferHandle handle, void *data) {
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
}*/

}  // namespace SirEngine::vk
