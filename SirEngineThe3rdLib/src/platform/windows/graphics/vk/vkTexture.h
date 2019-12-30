#pragma once
#include <vulkan/vulkan.h>

namespace SirEngine::vk {

struct VkTexture2D {
  uint32_t width;
  uint32_t height;
  uint32_t mipLevels;
  VkImage image;
  VkDeviceMemory deviceMemory;
  VkImageLayout imageLayout;
  VkImageView view;
  VkDescriptorImageInfo descriptor{};
  VkDescriptorImageInfo samplerOnly{};
};

bool createRenderTarget (const char *name, VkFormat format, VkDevice device,
                         VkTexture2D &outTexture,
                         VkImageUsageFlags imageUsageFlags,
                         VkImageLayout imageLayout, uint32_t width, uint32_t height); 

bool destroyFrameBuffer(const VkDevice device, const VkFramebuffer fb,
                        const VkTexture2D &texture); 

} // namespace vk