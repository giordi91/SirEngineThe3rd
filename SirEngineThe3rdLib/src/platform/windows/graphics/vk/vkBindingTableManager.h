#pragma once
#include <stdint.h>

#include "SirEngine/globals.h"
#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/memory/stackAllocator.h"
#include "platform/windows/graphics/vk/volk.h"

namespace SirEngine::vk {

class VkBindingTableManager final : public graphics::BindingTableManager {
  struct DescriptorData {
    uint16_t magicNumber;
    uint16_t isBuffered : 1;
    uint16_t padding : 15;
    VkDescriptorSet *sets;
    VkDescriptorSetLayout layout;
  };

  struct BindingTableData {
    graphics::BindingDescription *descriptions;
    VkDescriptorSetLayout layout;
    DescriptorHandle descriptorHandle;
	graphics::BINDING_TABLE_FLAGS flags;
    uint32_t magicNumber;
  };

  struct DescriptorPoolDefinition {
    uint32_t uniformDescriptorCount;
    uint32_t imagesDescriptorCount;
  };

 public:
  VkBindingTableManager(const uint32_t uniformDescriptorCount,
                        const uint32_t imagesDescriptorCount)
      : m_descriptorDataPool(RESERVE_SIZE),
        m_bindingTablePool(RESERVE_SIZE),
        m_uniformDescriptorCount(uniformDescriptorCount),
        m_imagesDescriptorCount(imagesDescriptorCount) {
    m_allocator.initialize(10 * MB_TO_BYTE);
  };
  ~VkBindingTableManager() = default;
  VkBindingTableManager(const VkBindingTableManager &) = delete;
  VkBindingTableManager &operator=(const VkBindingTableManager &) = delete;
  VkBindingTableManager(VkBindingTableManager &&) = delete;
  VkBindingTableManager &operator=(VkBindingTableManager &&) = delete;
  void initialize() override;
  void cleanup() override;

  DescriptorHandle allocate(const RSHandle handle,
                            const graphics::BINDING_TABLE_FLAGS flags,
                            const char *name);
  DescriptorHandle allocate(VkDescriptorSetLayout layout,
                            const graphics::BINDING_TABLE_FLAGS flags,
                            const char *name);

  [[nodiscard]] VkDescriptorSet_T *getDescriptorSet(
      const DescriptorHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const DescriptorData &data = m_descriptorDataPool.getConstRef(index);
    uint32_t setIndex = data.isBuffered ? globals::CURRENT_FRAME : 0;
    return data.sets[setIndex];
  }

  [[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout(
      const DescriptorHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const DescriptorData &data = m_descriptorDataPool.getConstRef(index);
    return data.layout;
  }

  bool isBuffered(const DescriptorHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const DescriptorData &data = m_descriptorDataPool.getConstRef(index);
    return data.isBuffered;
  }

  [[nodiscard]] VkDescriptorPool getPool() const { return m_descriptorPool; }

  BindingTableHandle allocateBindingTable(
      const graphics::BindingDescription *descriptions, const uint32_t count,
      graphics::BINDING_TABLE_FLAGS flags, const char *name = nullptr) override;

 private:
  inline void assertMagicNumber(const DescriptorHandle handle) const {
#ifdef SE_DEBUG
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_descriptorDataPool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for constant buffer");
#endif
  }

 private:
  static const uint32_t RESERVE_SIZE = 400;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  SparseMemoryPool<DescriptorData> m_descriptorDataPool;
  SparseMemoryPool<BindingTableData> m_bindingTablePool;
  uint32_t m_uniformDescriptorCount;
  uint32_t m_imagesDescriptorCount;
  VkDescriptorPool m_descriptorPool;
  StackAllocator m_allocator;
};

}  // namespace SirEngine::vk