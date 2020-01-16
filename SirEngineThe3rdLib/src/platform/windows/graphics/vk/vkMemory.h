#pragma once
#include <vulkan/vulkan.h>

namespace SirEngine::vk {

struct Buffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDescriptorBufferInfo info;
  void *data;
  size_t size;
  size_t allocationSize;
  uint32_t m_magicNumber;
};

// command buffers
VkCommandBuffer createCommandBuffer(const VkCommandPool pool ,const VkCommandBufferLevel level,
                                    const bool begin= false);

void flushCommandBuffer(VkCommandPool pool, VkCommandBuffer commandBuffer, const VkQueue queue,
                        const bool free);


} // namespace SirEngine::vk