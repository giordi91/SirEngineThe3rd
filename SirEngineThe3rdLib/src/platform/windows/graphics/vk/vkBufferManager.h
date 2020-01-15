#pragma once

#include "SirEngine/bufferManager.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/handle.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/randomSizeAllocator.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "vkMemory.h"

namespace SirEngine::vk {

class VkBufferManager final : public BufferManager {

public:
  static constexpr uint64_t SLAB_ALLOCATION_IN_MB = 256;
  static constexpr uint64_t MAX_ALLOCATED_SLABS = 10;

public:
  VkBufferManager() : BufferManager(), m_bufferStorage(RESERVE_SIZE) {}
  virtual ~VkBufferManager() = default;
  

  // deleted method to avoid copy, you can still move it though
  VkBufferManager(const VkBufferManager &) = delete;
  VkBufferManager &operator=(const VkBufferManager &) = delete;

  // override interface
  void initialize() override;
  void cleanup() override;
  BufferHandle allocate(const uint32_t sizeInBytes, void *initData,
                        const char *name, const int numElements,
                        const int elementSize, const uint32_t flags) override;

  void free(BufferHandle handle) override;
  BufferHandle allocateUpload(const uint32_t sizeInByte,
                              const uint32_t numElements,
                              const uint32_t elementSize,
                              const char *name) override {
    assert(0);
    return {};
  };

  const vk::Buffer &getBufferData(const BufferHandle handle) const {
    assertMagicNumber(handle);
    uint32_t idx = getIndexFromHandle(handle);
    return m_bufferStorage.getConstRef(idx);
  }

  VkBuffer getNativeBuffer(const BufferHandle& handle )const 
  {
    assertMagicNumber(handle);
    uint32_t idx = getIndexFromHandle(handle);
    return m_bufferStorage.getConstRef(idx).buffer;
  }

private:
  inline void assertMagicNumber(const BufferHandle handle) const {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_bufferStorage.getConstRef(idx).m_magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }

private:
  SparseMemoryPool<vk::Buffer> m_bufferStorage;
  static const uint32_t RESERVE_SIZE = 400;
  static const uint32_t RANDOM_ALLOC_RESERVE = 40 * 1024 * 1024;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  RandomSizeAllocator m_randomAlloc;
};

} // namespace SirEngine::vk
