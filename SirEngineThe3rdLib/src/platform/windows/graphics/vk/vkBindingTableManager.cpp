#include "platform/windows/graphics/vk/vkBindingTableManager.h"

#include <algorithm>

#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"

namespace SirEngine::vk {
DescriptorHandle VkBindingTableManager::allocate(
    const RSHandle handle, const graphics::BINDING_TABLE_FLAGS flags,
    const char *name) {
  VkDescriptorSetLayout layout =
      vk::PIPELINE_LAYOUT_MANAGER->getDescriptorSetLayoutFromHandle(handle);
  return allocate(layout, flags, name);
}

DescriptorHandle VkBindingTableManager::allocate(
    VkDescriptorSetLayout layout, const graphics::BINDING_TABLE_FLAGS flags,
    const char *name) {
  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = m_descriptorPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &layout;  // the layout we defined for the set,
                                       // so it also knows the size
  bool isBuffered =
      (flags & graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED) > 0;
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
  data.layout = layout;

  return {data.magicNumber << 16 | poolIdx};
}

inline VkDescriptorType getResourceType(
    const GRAPHIC_RESOURCE_TYPE resourceType) {
  switch (resourceType) {
    case GRAPHIC_RESOURCE_TYPE::READ_BUFFER:
      return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case GRAPHIC_RESOURCE_TYPE::READWRITE_BUFFER:
      return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER:
      return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case GRAPHIC_RESOURCE_TYPE::TEXTURE:
      return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    default:
      assert(0 && "resource not yet supported");
  }
}

inline bool isFlagSet(const uint32_t flags,
                      const GRAPHIC_RESOURCE_VISIBILITY toCheck) {
  return (flags & toCheck) > 0;
}

VkFlags getVisibilityFlags(const GRAPHIC_RESOURCE_VISIBILITY visibility) {
  VkFlags outFlags{};
  outFlags = isFlagSet(visibility, GRAPHICS_RESOURCE_VISIBILITY_VERTEX)
                 ? outFlags |= VK_SHADER_STAGE_VERTEX_BIT
                 : outFlags;
  outFlags = isFlagSet(visibility, GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT)
                 ? outFlags |= VK_SHADER_STAGE_FRAGMENT_BIT
                 : outFlags;
  outFlags = isFlagSet(visibility, GRAPHICS_RESOURCE_VISIBILITY_COMPUTE)
                 ? outFlags |= VK_SHADER_STAGE_COMPUTE_BIT
                 : outFlags;
  return outFlags;
}

BindingTableHandle VkBindingTableManager::allocateBindingTable(
    const graphics::BindingDescription *descriptions, const uint32_t count,
    const graphics::BINDING_TABLE_FLAGS flags, const char *name) {
  // allocate enough memory to host the bindings
  int allocSize = sizeof(VkDescriptorSetLayoutBinding) * count;
  // allocating enough memory of the set layout binding
  auto *bindings = reinterpret_cast<VkDescriptorSetLayoutBinding *>(
      globals::FRAME_ALLOCATOR->allocate(allocSize));
  // zeroing out
  memset(bindings, 0, allocSize);

  // now we convert the bindings from generic description to vulkan bindings
  for (uint32_t i = 0; i < count; ++i) {
    bindings[i].binding = descriptions[i].m_bindingIndex;
    bindings[i].descriptorType =
        getResourceType(descriptions[i].m_resourceType);
    bindings[i].descriptorCount = 1;
    bindings[i].stageFlags = getVisibilityFlags(descriptions[i].m_visibility);
  }

  // creating a layout
  VkDescriptorSetLayoutCreateInfo resourceLayoutInfo[1] = {};
  resourceLayoutInfo[0].sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  resourceLayoutInfo[0].pNext = nullptr;
  resourceLayoutInfo[0].bindingCount = 1;
  resourceLayoutInfo[0].pBindings = bindings;

  VkDescriptorSetLayout layout;
  VK_CHECK(vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, resourceLayoutInfo,
                                       NULL, &layout));
  SET_DEBUG_NAME(layout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, name);

  // now we have the layout we can allocate the descriptor set
  DescriptorHandle descriptorHandle = allocate(layout, flags, name);

  // we want to keep the description around for validation, we can possibly
  // disable this in release
  uint32_t descriptrionSize = sizeof(graphics::BindingDescription) * count;

  auto *descriptrionMemory = reinterpret_cast<graphics::BindingDescription *>(
      m_allocator.allocate(descriptrionSize));
  memcpy(descriptrionMemory, descriptions, descriptrionSize);
  // now that we interned the data we can generate the handle

  // store data in the pool
  uint32_t poolIdx;
  BindingTableData &data = m_bindingTablePool.getFreeMemoryData(poolIdx);
  data.layout = layout;
  data.magicNumber = MAGIC_NUMBER_COUNTER++;
  data.descriptions = descriptrionMemory;
  data.descriptorHandle = descriptorHandle;
  data.flags = flags;

  return {data.magicNumber << 16 | poolIdx};
}

void createDescriptorPool(const VkDevice device,
                          uint32_t uniformDescriptorCount,
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

void VkBindingTableManager::initialize() {
  createDescriptorPool(vk::LOGICAL_DEVICE, m_uniformDescriptorCount,
                       m_imagesDescriptorCount, m_descriptorPool);
}

void VkBindingTableManager::cleanup() {
  vkDestroyDescriptorPool(vk::LOGICAL_DEVICE, m_descriptorPool, nullptr);
}
}  // namespace SirEngine::vk