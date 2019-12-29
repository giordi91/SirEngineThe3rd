#pragma once
#include <stdint.h>

#include "../../../../../../Tests/src/graphNodesDefinitions.h"
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
  };

public:
  enum DESCRIPTOR_FLAGS { BUFFERED = 1 };

public:
  VkDescriptorManager() : m_descriptorPool(RESERVE_SIZE){};
  ~VkDescriptorManager() = default;
  VkDescriptorManager(const VkDescriptorManager &) = delete;
  VkDescriptorManager &operator=(const VkDescriptorManager &) = delete;
  VkDescriptorManager(VkDescriptorManager &&) = delete;
  VkDescriptorManager &operator=(VkDescriptorManager &&) = delete;

  DescriptorHandle allocate(const RSHandle handle, uint32_t flags,
                            const char *name);
  VkDescriptorSet getDescriptorSet(const DescriptorHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const DescriptorData &data = m_descriptorPool.getConstRef(index);
    uint32_t setIndex = data.isBuffered ? globals::CURRENT_FRAME : 0;
    return data.sets[setIndex];
  }

  void initialize(){};
  void cleanup(){};

private:
  inline void assertMagicNumber(const DescriptorHandle handle) const {
#ifdef SE_DEBUG
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_descriptorPool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for constant buffer");
#endif
  }

private:
  static const uint32_t RESERVE_SIZE = 400;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  SparseMemoryPool<DescriptorData> m_descriptorPool;
};

} // namespace SirEngine::vk