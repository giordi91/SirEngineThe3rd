#pragma once
#include <assert.h>

#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/memory/cpu/stringHashMap.h"
#include "SirEngine/rootSignatureManager.h"
#include "platform/windows/graphics/vk/volk.h"

namespace SirEngine::graphics {
struct MaterialMetadata;
}

namespace SirEngine::vk {

#define STATIC_SAMPLER_COUNT 7
extern VkSampler STATIC_SAMPLERS[STATIC_SAMPLER_COUNT];
extern VkDescriptorImageInfo STATIC_SAMPLERS_INFO[STATIC_SAMPLER_COUNT];
extern VkDescriptorSetLayout STATIC_SAMPLERS_LAYOUT;
extern VkDescriptorSet
    STATIC_SAMPLERS_DESCRIPTOR_SET;  // used in case you want to manually update
                                     // the samplers and not bound them as
                                     // static

extern VkDescriptorSetLayout PER_FRAME_LAYOUT;
extern DescriptorHandle PER_FRAME_DATA_HANDLE;
extern DescriptorHandle STATIC_SAMPLERS_HANDLE;
extern VkPipelineLayout ENGINE_PIPELINE_LAYOUT;

class VkPipelineLayoutManager final : public RootSignatureManager {
 public:
  VkPipelineLayoutManager()
      : m_rootRegister(RESERVE_SIZE), m_rsPool(RESERVE_SIZE){};
  VkPipelineLayoutManager(const VkPipelineLayoutManager &) = delete;
  VkPipelineLayoutManager &operator=(const VkPipelineLayoutManager &) = delete;
  ~VkPipelineLayoutManager() = default;
  void initialize() override;
  ;
  void cleanup() override;
  RSHandle loadSignatureFile(const char *name,
                             graphics::MaterialMetadata *metadata);

  inline VkPipelineLayout getLayoutFromName(const char *name) const {
    const RSHandle handle = getHandleFromName(name);
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const LayoutData &data = m_rsPool.getConstRef(index);
    return data.layout;
  }

  RSHandle getHandleFromName(const char *name) const override;

  [[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayoutFromHandle(
      RSHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const LayoutData &data = m_rsPool.getConstRef(index);
    return data.descriptorSetLayout;
  }

  inline VkPipelineLayout getLayoutFromHandle(const RSHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const LayoutData &data = m_rsPool.getConstRef(index);
    return data.layout;
  }

  static VkPipelineLayout createEngineLayout(
      const VkDescriptorSetLayout perFrameLayout,
      const VkDescriptorSetLayout samplersLayout);
  void bindGraphicsRS(const RSHandle) const override {}

 private:
  inline void assertMagicNumber(const RSHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_rsPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for pipeline layout");
  }

 private:
  struct LayoutData {
    VkPipelineLayout layout = nullptr;
    VkDescriptorSetLayout descriptorSetLayout = nullptr;
    VkDescriptorSetLayout passSetLayout = nullptr;
    uint32_t magicNumber : 16;
    uint32_t usesStaticSamplers : 16;
  };

  HashMap<const char *, RSHandle, hashString32> m_rootRegister;
  // handles
  SparseMemoryPool<LayoutData> m_rsPool;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
};

}  // namespace SirEngine::vk
