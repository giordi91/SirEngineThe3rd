#pragma once
#include <stdint.h>

#include "SirEngine/globals.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "platform/windows/graphics/vk/volk.h"

namespace SirEngine::vk {

class VkDescriptorManager final {
  struct DescriptorData {
    uint16_t magicNumber;
    uint16_t isBuffered : 1;
    uint16_t padding : 15;
    VkDescriptorSet *sets;
    VkDescriptorSetLayout layout;
  };

  struct DescriptorPoolDefinition {
    uint32_t uniformDescriptorCount;
    uint32_t imagesDescriptorCount;
  };

 public:
  enum DESCRIPTOR_FLAGS_BITS { BUFFERED = 1 };
  typedef uint32_t DESCRIPTOR_FLAGS;

 public:
  VkDescriptorManager(uint32_t uniformDescriptorCount,
                      uint32_t imagesDescriptorCount)
      : m_descriptorDataPool(RESERVE_SIZE),
        m_uniformDescriptorCount(uniformDescriptorCount),
        m_imagesDescriptorCount(imagesDescriptorCount){};
  ~VkDescriptorManager() = default;
  VkDescriptorManager(const VkDescriptorManager &) = delete;
  VkDescriptorManager &operator=(const VkDescriptorManager &) = delete;
  VkDescriptorManager(VkDescriptorManager &&) = delete;
  VkDescriptorManager &operator=(VkDescriptorManager &&) = delete;
  void initialize();
  void cleanup();

  DescriptorHandle allocate(const RSHandle handle, DESCRIPTOR_FLAGS flags,
                            const char *name);
  DescriptorHandle allocate(VkDescriptorSetLayout layout,
                            DESCRIPTOR_FLAGS flags, const char *name);

  VkDescriptorSet getDescriptorSet(const DescriptorHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const DescriptorData &data = m_descriptorDataPool.getConstRef(index);
    uint32_t setIndex = data.isBuffered ? globals::CURRENT_FRAME : 0;
    return data.sets[setIndex];
  }
  VkDescriptorSetLayout getDescriptorSetLayout(
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

  VkDescriptorPool getPool() const { return m_descriptorPool; }

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
  uint32_t m_uniformDescriptorCount;
  uint32_t m_imagesDescriptorCount;
  VkDescriptorPool m_descriptorPool;
};

}  // namespace SirEngine::vk