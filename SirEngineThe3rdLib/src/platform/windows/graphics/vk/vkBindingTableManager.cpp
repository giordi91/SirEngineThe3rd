#include "platform/windows/graphics/vk/vkBindingTableManager.h"

#include <algorithm>

#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "vkBufferManager.h"
#include "vkConstantBufferManager.h"
#include "vkMeshManager.h"
#include "vkTextureManager.h"

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
  data.magicNumber = static_cast<uint16_t>(MAGIC_NUMBER_COUNTER++);
  data.sets = sets;
  data.layout = layout;

  return {data.magicNumber << 16 | poolIdx};
}

inline VkDescriptorType getResourceType(
    const GRAPHIC_RESOURCE_TYPE resourceType, VkShaderStageFlags stage) {
  switch (resourceType) {
    case GRAPHIC_RESOURCE_TYPE::READ_BUFFER:
      return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case GRAPHIC_RESOURCE_TYPE::READWRITE_BUFFER:
      return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER:
      return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case GRAPHIC_RESOURCE_TYPE::TEXTURE: {
      bool isCompute =
          (stage & VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT) > 0;
      return isCompute ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                       : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }
    default:
      assert(0 && "resource not yet supported");
      return VK_DESCRIPTOR_TYPE_MAX_ENUM;
  }
}

inline bool isFlagSet(const uint32_t flags,
                      const GRAPHIC_RESOURCE_VISIBILITY_BITS toCheck) {
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
  auto *bindings = static_cast<VkDescriptorSetLayoutBinding *>(
      globals::FRAME_ALLOCATOR->allocate(allocSize));
  // zeroing out
  memset(bindings, 0, allocSize);

  // now we convert the bindings from generic description to vulkan bindings
  for (uint32_t i = 0; i < count; ++i) {
    bindings[i].stageFlags = getVisibilityFlags(descriptions[i].m_visibility);
    bindings[i].binding = descriptions[i].m_bindingIndex;
    bindings[i].descriptorType =
        getResourceType(descriptions[i].m_resourceType, bindings[i].stageFlags);
    bindings[i].descriptorCount = 1;
  }

  // creating a layout
  VkDescriptorSetLayoutCreateInfo resourceLayoutInfo[1] = {};
  resourceLayoutInfo[0].sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  resourceLayoutInfo[0].pNext = nullptr;
  resourceLayoutInfo[0].bindingCount = count;
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
  data.descriptionCount = count;
  data.flags = flags;

  return {data.magicNumber << 16 | poolIdx};
}

void VkBindingTableManager::bindTexture(const BindingTableHandle bindHandle,
                                        const TextureHandle texture,
                                        const uint32_t descriptorIndex,
                                        const uint32_t bindingIndex,
                                        const bool) {
  assertMagicNumber(bindHandle);
  uint32_t index = getIndexFromHandle(bindHandle);
  const BindingTableData &data = m_bindingTablePool.getConstRef(index);

  bool isCompute = false;
  for (uint32_t i = 0; i < data.descriptionCount; ++i) {
    bool isTexture =
        data.descriptions[i].m_resourceType == GRAPHIC_RESOURCE_TYPE::TEXTURE;
    bool isCorrectIndex = data.descriptions[i].m_bindingIndex == bindingIndex;
    bool visibilityCompute = data.descriptions[i].m_visibility ==
                             GRAPHICS_RESOURCE_VISIBILITY_COMPUTE;
    bool finalCondition = isTexture & isCorrectIndex & visibilityCompute;
    isCompute |= finalCondition;
  }

  assert(data.descriptorHandle.isHandleValid());
  // the descriptor set is already taking into account whether or not
  // is buffered, it gives us the correct one we want
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(data.descriptorHandle);

  // assert(!vk::DESCRIPTOR_MANAGER->isBuffered(descriptorHandle) &&
  //       "buffered not yet implemented");

  VkWriteDescriptorSet writeDescriptorSets{};

  vk::TEXTURE_MANAGER->bindTexture(texture, &writeDescriptorSets, descriptorSet,
                                   bindingIndex, isCompute);

  // Execute the writes to update descriptors for this set
  // Note that it's also possible to gather all writes and only run updates
  // once, even for multiple sets This is possible because each
  // VkWriteDescriptorSet also contains the destination set to be updated
  // For simplicity we will update once per set instead
  // object one off update
  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 1, &writeDescriptorSets, 0,
                         nullptr);
}

void VkBindingTableManager::bindMesh(const BindingTableHandle bindHandle,
                                     const MeshHandle mesh,
                                     const uint32_t startIndex,
                                     const MESH_ATTRIBUTE_FLAGS meshFlags) {
  assertMagicNumber(bindHandle);
  uint32_t index = getIndexFromHandle(bindHandle);
  const auto &data = m_bindingTablePool.getConstRef(index);

  assert(data.descriptorHandle.isHandleValid());
  // the descriptor set is already taking into account whether or not
  // is buffered, it gives us the correct one we want
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(data.descriptorHandle);

  VkWriteDescriptorSet writeDescriptorSets[4] = {};
  VkDescriptorBufferInfo bufferInfo[4] = {};
  /*
      vk::BUFFER_MANAGER->bindBuffer(buffer, &writeDescriptorSets,
     descriptorSet, bindingIndex);
                                 */

  vk::MESH_MANAGER->bindMesh(mesh, writeDescriptorSets, descriptorSet,
                             bufferInfo, meshFlags, startIndex);
  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 4, writeDescriptorSets, 0,
                         nullptr);
}

void VkBindingTableManager::bindConstantBuffer(
    const BindingTableHandle &bindingTable,
    const ConstantBufferHandle &constantBufferHandle,
    const uint32_t descriptorIndex, const uint32_t bindingIndex) {
  assertMagicNumber(bindingTable);
  uint32_t index = getIndexFromHandle(bindingTable);
  const auto &data = m_bindingTablePool.getConstRef(index);

  assert(data.descriptorHandle.isHandleValid());
  // the descriptor set is already taking into account whether or not
  // is buffered, it gives us the correct one we want
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(data.descriptorHandle);

  VkWriteDescriptorSet writeDescriptorSets = {};
  VkDescriptorBufferInfo bufferInfoUniform = {};
  vk::CONSTANT_BUFFER_MANAGER->bindConstantBuffer(
      constantBufferHandle, bufferInfoUniform, bindingIndex,
      &writeDescriptorSets, descriptorSet);

  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 1, &writeDescriptorSets, 0,
                         nullptr);
}

void VkBindingTableManager::bindBuffer(const BindingTableHandle bindHandle,
                                       const BufferHandle buffer,
                                       const uint32_t descriptorIndex,
                                       const uint32_t bindingIndex) {
  assertMagicNumber(bindHandle);
  uint32_t index = getIndexFromHandle(bindHandle);
  const auto &data = m_bindingTablePool.getConstRef(index);

  assert(data.descriptorHandle.isHandleValid());
  // the descriptor set is already taking into account whether or not
  // is buffered, it gives us the correct one we want
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(data.descriptorHandle);

  VkWriteDescriptorSet writeDescriptorSets = {};
  VkDescriptorBufferInfo bufferInfoUniform = {};
  vk::BUFFER_MANAGER->bindBuffer(buffer, &writeDescriptorSets, descriptorSet,
                                 bindingIndex);

  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 1, &writeDescriptorSets, 0,
                         nullptr);
}

void VkBindingTableManager::bindTable(const uint32_t bindingSpace,
                                      const BindingTableHandle bindHandle,
                                      const RSHandle rsHandle, bool isCompute) {
  assertMagicNumber(bindHandle);
  uint32_t index = getIndexFromHandle(bindHandle);
  const auto &data = m_bindingTablePool.getConstRef(index);

  DescriptorHandle descriptorHandle = data.descriptorHandle;
  // data.m_materialRuntime.descriptorHandles[currentFlagId];
  assert(descriptorHandle.isHandleValid());
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(descriptorHandle);

  VkPipelineLayout layout =
      vk::PIPELINE_LAYOUT_MANAGER->getLayoutFromHandle(rsHandle);
  assert(layout != nullptr);

  VkDescriptorSet sets[] = {
      descriptorSet,
  };
  VkPipelineBindPoint bindPoint = isCompute ? VK_PIPELINE_BIND_POINT_COMPUTE
                                            : VK_PIPELINE_BIND_POINT_GRAPHICS;
  // multiple descriptor sets
  vkCmdBindDescriptorSets(CURRENT_FRAME_COMMAND->m_commandBuffer, bindPoint,
                          layout, bindingSpace, 1, sets, 0, nullptr);
}

void VkBindingTableManager::free(const BindingTableHandle &bindingTable) {
  assertMagicNumber(bindingTable);
  uint32_t index = getIndexFromHandle(bindingTable);
  const auto &data = m_bindingTablePool.getConstRef(index);

  vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE, data.layout, nullptr);
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
