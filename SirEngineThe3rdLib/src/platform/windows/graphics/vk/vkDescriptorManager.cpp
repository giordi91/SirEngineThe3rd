#include "platform/windows/graphics/vk/vkDescriptorManager.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"
#include <algorithm>

namespace SirEngine::vk {
DescriptorHandle VkDescriptorManager::allocate(const RSHandle handle,
                                               const uint32_t flags,
                                               const char *name) {
  VkDescriptorSetLayout layout =
      vk::PIPELINE_LAYOUT_MANAGER->getDescriptorSetLayoutFromHandle(handle);
  return allocate(layout, flags, name);
}

DescriptorHandle VkDescriptorManager::allocate(VkDescriptorSetLayout layout,
                                               const uint32_t flags,
                                               const char *name) {
  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = m_descriptorPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &layout; // the layout we defined for the set,
                                      // so it also knows the size
  bool isBuffered = (flags & DESCRIPTOR_FLAGS::BUFFERED) > 0;
  uint32_t count = isBuffered ? vk::SWAP_CHAIN_IMAGE_COUNT : 1;

  // allocate enough memory for the sets
  auto sets = reinterpret_cast<VkDescriptorSet *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(VkDescriptorSet) * count));

  char countStr[2];
  countStr[1] = '\0';
  assert(count <= 9);
  for (uint32_t i = 0; i < count; ++i) {
    VK_CHECK(
        vkAllocateDescriptorSets(vk::LOGICAL_DEVICE, &allocateInfo, &sets[i]));
    countStr[0] = static_cast<char>(i + 48);
    if (name != nullptr) {
      SET_DEBUG_NAME(sets[i], VK_OBJECT_TYPE_DESCRIPTOR_SET,
                     frameConcatenation(
                         name, frameConcatenation("DescriptorSet", countStr)));
    }
  }

  // store data in the pool
  uint32_t poolIdx;
  DescriptorData &data = m_descriptorDataPool.getFreeMemoryData(poolIdx);
  data.isBuffered = isBuffered;
  data.magicNumber = MAGIC_NUMBER_COUNTER++;
  data.sets = sets;

  return {data.magicNumber << 16 | poolIdx};
}

void createDescriptorPool(VkDevice device, uint32_t uniformDescriptorCount,
                          uint32_t imagesDescriptorCount,
                          VkDescriptorPool &descriptorPool) {

  VkDescriptorPoolSize descriptorPoolSizes[3]{};

  // Uniform buffers : 1 for scene and 1 per object (scene and local matrices)
  descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorPoolSizes[0].descriptorCount = uniformDescriptorCount;

  // Combined image samples : 1 per mesh texture
  descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  descriptorPoolSizes[1].descriptorCount = imagesDescriptorCount;

  // samplers
  descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
  descriptorPoolSizes[2].descriptorCount = imagesDescriptorCount;

  // Create the global descriptor pool
  VkDescriptorPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  createInfo.poolSizeCount = ARRAYSIZE(descriptorPoolSizes);
  createInfo.pPoolSizes = descriptorPoolSizes;
  // Max. number of descriptor sets that can be allocted from this
  // pool (one per object)
  createInfo.maxSets = uniformDescriptorCount > imagesDescriptorCount
                           ? uniformDescriptorCount
                           : imagesDescriptorCount;

  VK_CHECK(
      vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool));
}

void VkDescriptorManager::initialize() {
  createDescriptorPool(vk::LOGICAL_DEVICE, m_uniformDescriptorCount,
                       m_imagesDescriptorCount, m_descriptorPool);
}

void VkDescriptorManager::cleanup() {
  vkDestroyDescriptorPool(vk::LOGICAL_DEVICE, m_descriptorPool, nullptr);
}
} // namespace SirEngine::vk
