#pragma once

#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/memory/stackAllocator.h"
#include "platform/windows/graphics/dx12/DX12.h"

namespace SirEngine::dx12 {

class Dx12BindingTableManager : public graphics::BindingTableManager {
  struct BindingTableData {
    graphics::BindingDescription* descriptions;
    // this is the flat array of descriptors for all buffered frame
    // included, to get frame 1 for example you will need to shift by the number
    // of descriptions
    DescriptorPair* descriptors;
    graphics::BINDING_TABLE_FLAGS flags;
    uint32_t magicNumber;
    uint32_t descriptionsCount;
  };

 public:
  Dx12BindingTableManager() : m_bindingTablePool(RESERVE_SIZE) {
    m_allocator.initialize(10 * MB_TO_BYTE);
  };
  virtual ~Dx12BindingTableManager() = default;
  void initialize() override;
  void cleanup() override;
  BindingTableHandle allocateBindingTable(
      const graphics::BindingDescription* descriptions, const uint32_t count,
      graphics::BINDING_TABLE_FLAGS flags, const char* name) override;
  void bindTexture(const BindingTableHandle bindHandle,
                   const TextureHandle texture, const uint32_t descriptorIndex,
                   const uint32_t bindingIndex, const bool isCube) override;

  void bindTable(const BindingTableHandle bindHandle,
                 const PSOHandle psoHandle) override;

 private:
  inline void assertMagicNumber(const BindingTableHandle handle) {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(m_bindingTablePool[idx].magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }

 private:
  static const uint32_t RESERVE_SIZE = 400;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  SparseMemoryPool<BindingTableData> m_bindingTablePool;
  StackAllocator m_allocator;
};

}  // namespace SirEngine::dx12