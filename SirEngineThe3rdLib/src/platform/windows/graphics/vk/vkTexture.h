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
  VkSampler sampler;
  VkImageView view;
  VkDescriptorImageInfo descriptor{};
  VkDescriptorImageInfo samplerOnly{};
};

bool loadTextureFromFile(
    const char *name, VkFormat format, VkDevice device, 
    VkTexture2D &outTexture,
    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

bool destroyTexture(VkDevice device, VkTexture2D texture);

} // namespace vk