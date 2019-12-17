#pragma once
#include <vulkan/vulkan.h>

namespace SirEngine::vk {

struct Buffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  void *data;
  size_t size;
};

// command buffers
VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level,
                                    bool begin = false);

void flushCommandBuffer(VkCommandBuffer commandBuffer, const VkQueue queue,
                        const bool free);
// buffers
uint32_t
selectMemoryType(const VkPhysicalDeviceMemoryProperties &memoryProperties,
                 const uint32_t memoryTypeBits,
                 const VkMemoryPropertyFlags flags);

void createBuffer(Buffer &buffer, const VkDevice device,
                  const VkPhysicalDeviceMemoryProperties &memoryProperties,
                  size_t size, const VkBufferUsageFlags usage,const char* name);
void destroyBuffer(VkDevice device, const Buffer &buffer);

} // namespace SirEngine::vk