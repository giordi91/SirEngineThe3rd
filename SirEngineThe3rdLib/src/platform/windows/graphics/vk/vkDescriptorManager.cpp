#include "platform/windows/graphics/vk/vkDescriptorManager.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"

namespace SirEngine::vk {
DescriptorHandle VkDescriptorManager::allocate(const RSHandle handle,
                                               uint32_t flags,
                                               const char *name) {
  VkDescriptorSetLayout layout =
      vk::PIPELINE_LAYOUT_MANAGER->getDescriptorSetLayoutFromHandle(handle);

  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = vk::DESCRIPTOR_POOL;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &layout; // the layout we defined for the set,
                                      // so it also knows the size
  bool isBuffered = (flags & DESCRIPTOR_FLAGS::BUFFERED) > 0;
  int count = isBuffered ? vk::SWAP_CHAIN_IMAGE_COUNT : 1;

  // allocate enough memory for the sets
  auto sets = reinterpret_cast<VkDescriptorSet *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(VkDescriptorSet) * count));

  char countStr[2];
  countStr[1] = '\0';
  assert(count <= 9);
  for (int i = 0; i < count; ++i) {
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
  DescriptorData &data = m_descriptorPool.getFreeMemoryData(poolIdx);
  data.isBuffered = isBuffered;
  data.magicNumber = MAGIC_NUMBER_COUNTER++;
  data.sets = sets;

  return {data.magicNumber << 16 | poolIdx};
}
} // namespace SirEngine::vk
