#pragma once
#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/memory/stringHashMap.h"
#include "SirEngine/rootSignatureManager.h"

#include "vk.h"

#include <cassert>

namespace SirEngine::vk {

class VkPipelineLayoutManager final : public RootSignatureManager {

public:
  VkPipelineLayoutManager()
      : m_rootRegister(RESERVE_SIZE), m_rsPool(RESERVE_SIZE){};
  VkPipelineLayoutManager(const VkPipelineLayoutManager &) = delete;
  VkPipelineLayoutManager &operator=(const VkPipelineLayoutManager &) = delete;
  ~VkPipelineLayoutManager() = default;
  void initialize() override{};
  void cleanup() override{};
  void loadSignaturesInFolder(const char *directory) override;
  void loadSignatureBinaryFile(const char *file) override;
  RSHandle loadSignatureFile(const char *file,
                             VkDescriptorSetLayout samplersLayout);

  inline VkPipelineLayout getLayoutFromName(const char *name) const {

    const RSHandle handle = getHandleFromName(name);
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const LayoutData &data = m_rsPool.getConstRef(index);
    return data.layout;
  }
  inline VkPipelineLayout getLayoutFromHandle(const RSHandle handle) const {

    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const LayoutData &data = m_rsPool.getConstRef(index);
    return data.layout;
  }

  inline void bindGraphicsRS(const RSHandle handle,
                             VkCommandBuffer *commandList) const {

    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const LayoutData &data = m_rsPool.getConstRef(index);
    assert(0);
    // commandList->SetGraphicsLayout(data.rs);
  }

  RSHandle getHandleFromName(const char *name) const override {
    assert(m_rootRegister.containsKey(name));
    RSHandle value;
    m_rootRegister.get(name, value);
    return value;
  }

  // mostly to keep API uniform
  void init(){};

private:
  inline uint32_t getIndexFromHandle(const RSHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(const RSHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }
  inline void assertMagicNumber(const RSHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_rsPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for pipeline layout");
  }

private:
  struct LayoutData {
    VkPipelineLayout layout;
    uint32_t magicNumber;
  };

  HashMap<const char *, RSHandle, hashString32> m_rootRegister;
  // handles
  SparseMemoryPool<LayoutData> m_rsPool;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
};

} // namespace SirEngine::vk
