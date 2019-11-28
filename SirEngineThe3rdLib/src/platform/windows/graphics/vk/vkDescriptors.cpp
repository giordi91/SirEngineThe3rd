#include "platform/windows/graphics/vk/vkDescriptors.h"
#include "platform/windows/graphics/vk/vk.h"
#include "algorithm"

#undef min
namespace SirEngine::vk {
void createDescriptorPool(VkDevice device,
                          const DescriptorPoolDefinition &definition,
                          VkDescriptorPool &descriptorPool) {

  VkDescriptorPoolSize descriptorPoolSizes[3]{};

  // Uniform buffers : 1 for scene and 1 per object (scene and local matrices)
  descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorPoolSizes[0].descriptorCount = definition.uniformDescriptorCount;

  // Combined image samples : 1 per mesh texture
  descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  descriptorPoolSizes[1].descriptorCount = definition.imagesDescriptorCount;

  //samplers
  descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
  descriptorPoolSizes[2].descriptorCount = definition.imagesDescriptorCount;

  // Create the global descriptor pool
  VkDescriptorPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  createInfo.poolSizeCount = ARRAYSIZE(descriptorPoolSizes);
  createInfo.pPoolSizes = descriptorPoolSizes;
  // Max. number of descriptor sets that can be allocted from this
  // pool (one per object)
  createInfo.maxSets = std::min(definition.uniformDescriptorCount,
                                definition.imagesDescriptorCount);

  VK_CHECK(
      vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool));
}





} // namespace vk
