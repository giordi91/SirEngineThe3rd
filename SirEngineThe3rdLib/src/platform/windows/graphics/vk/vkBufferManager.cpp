#include "platform/windows/graphics/vk/vkBufferManager.h"

#include "platform/windows/graphics/vk/vk.h"

namespace SirEngine::vk {
static const VkAccessFlags FLAGS_TO_ACCESS_BITS[] = {
    VK_ACCESS_FLAG_BITS_MAX_ENUM, VK_ACCESS_MEMORY_WRITE_BIT,
    VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT};

static const VkPipelineStageFlags FLAGS_TO_STAGE_BITS[] = {
    VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT};

void VkBufferManager::initialize() {
  m_randomAlloc.initialize(RANDOM_ALLOC_RESERVE);
}

void VkBufferManager::cleanup() {}

void freeBuffer(const vk::Buffer &buffer) {
  vkFreeMemory(vk::LOGICAL_DEVICE, buffer.memory, nullptr);
  vkDestroyBuffer(vk::LOGICAL_DEVICE, buffer.buffer, nullptr);
}
void VkBufferManager::free(const BufferHandle handle) {
  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  VkBufferInfo &buffData = m_bufferStorage[idx];

  bool isStatic = (buffData.flags & BUFFER_FLAGS_BITS::IS_STATIC) > 0;
  bool isGPUOnly = (buffData.flags & BUFFER_FLAGS_BITS::GPU_ONLY) > 0;
  freeBuffer(buffData.cpuBuffer);

  if (isGPUOnly) {
    freeBuffer(buffData.gpuBuffer);
  }
  buffData = {};
  // next we just need to free the pool
  m_bufferStorage.free(idx);
}

void *VkBufferManager::getMappedData(const BufferHandle handle) const {
  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  const VkBufferInfo &data = m_bufferStorage.getConstRef(idx);
  return data.cpuBuffer.data;
}

void VkBufferManager::bindBuffer(const BufferHandle handle,
                                 VkWriteDescriptorSet *write,
                                 const VkDescriptorSet set,
                                 const uint32_t bindingIndex) const {
  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  const VkBufferInfo &data = m_bufferStorage.getConstRef(idx);

  write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write->dstSet = set;
  write->dstBinding = bindingIndex;

  bool isGPUOnly = (data.flags & BUFFER_FLAGS_BITS::GPU_ONLY) > 0;
  // TODO this should be probably be queried by the buffer data
  write->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write->pBufferInfo = isGPUOnly ? &data.gpuBuffer.info : &data.cpuBuffer.info;
  write->descriptorCount = 1;
}

void VkBufferManager::update(BufferHandle handle, void *inData, int offset,
                             int size) {
  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  const VkBufferInfo &data = m_bufferStorage.getConstRef(idx);

  int actualOffset = size == -1 ? 0 : offset;
  int actualSize = size == -1 ? data.cpuBuffer.size : size;
  // we copy to the cpu memory
  memcpy(static_cast<char *>(data.cpuBuffer.data) + actualOffset,
         static_cast<char *>(inData) + actualOffset, actualSize);

  bool isGPUOnly = (data.flags & BUFFER_FLAGS_BITS::GPU_ONLY) > 0;
  if (isGPUOnly) {
    // we need to perform a buffer to buffer copy
    auto *currentFc = CURRENT_FRAME_COMMAND;
    VkCommandBuffer commandList = currentFc->m_commandBuffer;

    VkBufferCopy copyRegion = {};
    copyRegion.size = actualSize;
    vkCmdCopyBuffer(commandList, data.cpuBuffer.buffer, data.gpuBuffer.buffer,
                    1, &copyRegion);

    VkBufferMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        nullptr,
        VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT,
        VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        data.gpuBuffer.buffer,
        actualOffset,
        actualSize};
    vkCmdPipelineBarrier(
        commandList, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
        // TODO Need to fix this, might need extra information passed int he
        // flags to know what the buffer is used for, for example a compute
        // buffer etc, or maybe I should leave the copy to the user through an
        // interface?
        VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        0, 0, nullptr, 1, &barrier, 0, nullptr);
  }
}

void VkBufferManager::transitionBuffer(const BufferHandle handle,
                                       const BufferTransition &transition) {
  const Buffer &bufferData = getBufferData(handle);
  VkBufferMemoryBarrier barrier{
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
      nullptr,
      FLAGS_TO_ACCESS_BITS[transition.sourceState],
      FLAGS_TO_ACCESS_BITS[transition.destinationState],
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      bufferData.buffer,
      0,
      bufferData.size};
  vkCmdPipelineBarrier(vk::CURRENT_FRAME_COMMAND->m_commandBuffer,
                       FLAGS_TO_STAGE_BITS[transition.sourceStage],
                       FLAGS_TO_STAGE_BITS[transition.destinationStage],
                       VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &barrier, 0,
                       nullptr);
}

vk::Buffer VkBufferManager::allocateGPUVisibleBuffer(
    const uint32_t sizeInBytes, const VkBufferUsageFlags usage,
    const char *name, void *initData) {
  VkBufferCreateInfo createInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  createInfo.size = sizeInBytes;
  createInfo.usage = usage;
  createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  vk::Buffer buffer{};

  // this is just a dummy handle
  VK_CHECK(
      vkCreateBuffer(vk::LOGICAL_DEVICE, &createInfo, nullptr, &buffer.buffer));
  SET_DEBUG_NAME(buffer.buffer, VK_OBJECT_TYPE_BUFFER,
                 frameConcatenation(name, "Buffer"));

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

  // only memory we need is device local, since is a GPU only buffer
  VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  uint32_t memoryIndex = selectMemoryType(
      memoryProperties, requirements.memoryTypeBits, memoryFlags);

  assert(memoryIndex != ~0u);

  VkMemoryAllocateInfo memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memoryInfo.allocationSize = requirements.size;
  memoryInfo.memoryTypeIndex = memoryIndex;

  VK_CHECK(vkAllocateMemory(vk::LOGICAL_DEVICE, &memoryInfo, nullptr,
                            &buffer.memory));
  SET_DEBUG_NAME(buffer.memory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                 frameConcatenation(name, "Memory"));

  // binding the memory to our buffer, the dummy handle we allocated previously
  vkBindBufferMemory(vk::LOGICAL_DEVICE, buffer.buffer, buffer.memory, 0);

  buffer.m_magicNumber = MAGIC_NUMBER_COUNTER++;
  buffer.info.buffer = buffer.buffer;
  buffer.info.offset = 0;
  buffer.info.range = sizeInBytes;

  return buffer;
}

vk::Buffer VkBufferManager::allocateCPUVisibleBuffer(uint32_t sizeInBytes,
                                                     VkBufferUsageFlags usage,
                                                     const char *name,
                                                     void *initData) {
  VkBufferCreateInfo createInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  createInfo.size = sizeInBytes;
  createInfo.usage = usage;
  createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  vk::Buffer buffer{};

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
  // TODO do this only if we request fast cpu update or similar
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
  buffer.info.range = sizeInBytes;
  return buffer;
}

BufferHandle VkBufferManager::allocate(const uint32_t sizeInBytes,
                                       void *initData, const char *name,
                                       const int, const int,
                                       const BUFFER_FLAGS flags) {
  bool isRandomWrite = (flags & BUFFER_FLAGS_BITS::RANDOM_WRITE) > 0;
  bool isIndex = (flags & BUFFER_FLAGS_BITS::INDEX_BUFFER) > 0;
  bool isIndirectBuffer = (flags & BUFFER_FLAGS_BITS::INDIRECT_BUFFER) > 0;
  bool isVertexBuffer = (flags & BUFFER_FLAGS_BITS::VERTEX_BUFFER) > 0;
  bool isBuffered = (flags & BUFFER_FLAGS_BITS::BUFFERED) > 0;
  bool isStorage = (flags & BUFFER_FLAGS_BITS::STORAGE_BUFFER) > 0;
  bool isGPUOnly = (flags & BUFFER_FLAGS_BITS::GPU_ONLY) > 0;
  bool isStatic = (flags & BUFFER_FLAGS_BITS::IS_STATIC) > 0;
  assert(!isBuffered && "not supported yet");
  // assert(!isIndirectBuffer && "not supported yet");
  assert(!(isVertexBuffer && isIndex) &&
         "canont be both vertex and index buffer");

  // process the flags
  uint32_t usage = isRandomWrite ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
  usage |= isIndex ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
  usage |= isIndirectBuffer ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0;
  usage |=
      (isVertexBuffer | isStorage) > 0 ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
  usage |= isIndirectBuffer ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0;

  uint32_t cpuUsage = usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  uint32_t gpuUsage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  // we start by allocating a cpu buffer because is going to be needed no matter
  // if we perform an upload to gpu or not
  vk::Buffer cpuBuffer =
      allocateCPUVisibleBuffer(sizeInBytes, cpuUsage, name, initData);

  // if is GPUOnly we need to create a gpu only buffer
  vk::Buffer gpuBuffer{};
  if (isGPUOnly) {
    gpuBuffer = allocateGPUVisibleBuffer(
        sizeInBytes, gpuUsage, frameConcatenation(name, "GPU"), initData);
    if (initData != nullptr) {
      // we need to perform a buffer to buffer copy
      auto *currentFc = CURRENT_FRAME_COMMAND;
      VkCommandBuffer commandList = currentFc->m_commandBuffer;

      VkBufferCopy copyRegion = {};
      copyRegion.size = sizeInBytes;
      vkCmdCopyBuffer(commandList, cpuBuffer.buffer, gpuBuffer.buffer, 1,
                      &copyRegion);

      VkBufferMemoryBarrier barrier{
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
          nullptr,
          VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT,
          VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT,
          VK_QUEUE_FAMILY_IGNORED,
          VK_QUEUE_FAMILY_IGNORED,
          gpuBuffer.buffer,
          0,
          sizeInBytes};
      vkCmdPipelineBarrier(
          commandList, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
          // TODO Need to fix this, might need extra information passed int he
          // flags to know what the buffer is used for, for example a compute
          // buffer etc, or maybe I should leave the copy to the user through an
          // interface?
          VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
              VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
          0, 0, nullptr, 1, &barrier, 0, nullptr);
    }
  }

  uint32_t index = 0;
  VkBufferInfo &bufferInfo = m_bufferStorage.getFreeMemoryData(index);
  // TODO we don't want to keep the cpu buffer around if the buffer is static,
  // this means keeping track of buffer uploaded around and release them after
  // data reached destination
  bufferInfo.cpuBuffer = cpuBuffer;
  bufferInfo.gpuBuffer = gpuBuffer;
  bufferInfo.flags = flags;
  bufferInfo.magicNumber = MAGIC_NUMBER_COUNTER++;
  // creating a handle
  BufferHandle handle{(bufferInfo.magicNumber << 16) | index};
  return handle;
}

}  // namespace SirEngine::vk
