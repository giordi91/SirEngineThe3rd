#pragma once
#include <vulkan/vulkan.h>

namespace SirEngine::vk {
struct DescriptorPoolDefinition {
  uint32_t uniformDescriptorCount;
  uint32_t imagesDescriptorCount;
};
void createDescriptorPool(VkDevice device,
                          const DescriptorPoolDefinition &definition,
                          VkDescriptorPool &descriptorPool);

} // namespace vk
