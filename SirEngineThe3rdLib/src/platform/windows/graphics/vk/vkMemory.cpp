#include "platform/windows/graphics/vk/vkMemory.h"
#include "platform/windows/graphics/vk/vk.h"
#include <cassert>

namespace SirEngine::vk {

VkCommandBuffer createCommandBuffer(const VkCommandPool pool,
                                    const VkCommandBufferLevel level,
                                    const bool begin) {
  VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
  cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdBufAllocateInfo.commandPool = pool;
  cmdBufAllocateInfo.level = level;
  cmdBufAllocateInfo.commandBufferCount = 1;

  VkCommandBuffer cmdBuffer;
  VK_CHECK(vkAllocateCommandBuffers(LOGICAL_DEVICE, &cmdBufAllocateInfo,
                                    &cmdBuffer));

  // If requested, also start recording for the new command buffer
  if (begin) {
    VkCommandBufferBeginInfo cmdBufInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
  }

  return cmdBuffer;
}
void flushCommandBuffer(VkCommandPool pool, VkCommandBuffer commandBuffer,
                        const VkQueue queue, const bool free) {
  if (commandBuffer == VK_NULL_HANDLE) {
    return;
  }

  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  // Create fence to ensure that the command buffer has finished executing
  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = 0;
  VkFence fence;
  VK_CHECK(vkCreateFence(LOGICAL_DEVICE, &fenceInfo, nullptr, &fence));

  // Submit to the queue
  VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence));
  // Wait for the fence to signal that command buffer has finished executing

  VK_CHECK(vkWaitForFences(LOGICAL_DEVICE, 1, &fence, VK_TRUE, 100000000000));

  vkDestroyFence(LOGICAL_DEVICE, fence, nullptr);

  if (free) {
    vkFreeCommandBuffers(LOGICAL_DEVICE, pool, 1, &commandBuffer);
  }
}

uint32_t
selectMemoryType(const VkPhysicalDeviceMemoryProperties &memoryProperties,
                 const uint32_t memoryTypeBits,
                 const VkMemoryPropertyFlags flags) {
  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
    uint32_t matchMemoryType = (memoryTypeBits & (1 << i)) != 0;
    uint32_t matchWantedFlags =
        (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags;
    if (matchMemoryType && (matchWantedFlags)) {
      return i;
    }
  }
  assert(!"No compatible memory type found");
  return ~0u;
}

void createBuffer(Buffer &buffer, const VkDevice device, const size_t size,
                  const VkBufferUsageFlags usage,
                  const VkMemoryPropertyFlags memoryFlags, const char *name) {

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
  const uint32_t memoryIndex =
      selectMemoryType(memoryProperties, requirements.memoryTypeBits,
                       memoryFlags);

  assert(memoryIndex != ~0u);

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

} // namespace vk

void destroyBuffer(const VkDevice device, const Buffer &buffer) {
  vkFreeMemory(device, buffer.memory, nullptr);
  vkDestroyBuffer(device, buffer.buffer, nullptr);
}
} // namespace SirEngine::vk
