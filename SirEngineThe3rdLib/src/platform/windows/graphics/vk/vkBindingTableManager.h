#pragma once
#include <stdint.h>

#include "SirEngine/globals.h"
#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/memory/cpu/stackAllocator.h"
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
    uint32_t descriptionCount;
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
	    m_imagesDescriptorCount(imagesDescriptorCount)
  {
	  m_allocator.initialize(10 * MB_TO_BYTE);
  }
  ~VkBindingTableManager() override = default;
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

  void bindTexture(const BindingTableHandle bindHandle,
                   const TextureHandle texture, const uint32_t descriptorIndex,
                   const uint32_t bindingIndex, const bool isCube) override;
  void bindMesh(const BindingTableHandle bindHandle, const MeshHandle mesh,
	  const MESH_ATTRIBUTE_FLAGS meshFlags) override;

  void bindConstantBuffer(const BindingTableHandle& bindingTable, const ConstantBufferHandle& constantBufferHandle,
	  const uint32_t descriptorIndex, const uint32_t bindingIndex) override;
  void bindBuffer(const BindingTableHandle bindHandle, const BufferHandle buffer, const uint32_t descriptorIndex,
	  const uint32_t bindingIndex) override;

  void bindTable(uint32_t bindingSpace, const BindingTableHandle bindHandle,
                 const RSHandle rsHandle, bool isCompute =false) override;
  void free(const BindingTableHandle& bindingTable) override;

 private:
  inline void assertMagicNumber(const DescriptorHandle handle) const {
#ifdef SE_DEBUG
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_descriptorDataPool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for descriptor handle buffer");
#endif
  }
  inline void assertMagicNumber(const BindingTableHandle handle) const {
#ifdef SE_DEBUG
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_bindingTablePool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for binding table pool");
#endif
  }

public:
 private:
  static const uint32_t RESERVE_SIZE = 400;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  SparseMemoryPool<DescriptorData> m_descriptorDataPool;
  SparseMemoryPool<BindingTableData> m_bindingTablePool;
  uint32_t m_uniformDescriptorCount;
  uint32_t m_imagesDescriptorCount;
  VkDescriptorPool m_descriptorPool{};
  StackAllocator m_allocator;
};

}  // namespace SirEngine::vk