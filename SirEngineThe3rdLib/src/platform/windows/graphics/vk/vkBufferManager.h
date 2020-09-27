#pragma once

#include "SirEngine/bufferManager.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/handle.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/cpu/randomSizeAllocator.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "vkMemory.h"

namespace SirEngine::vk {

class VkBufferManager final : public BufferManager {
  struct VkBufferInfo {
    vk::Buffer cpuBuffer;
    vk::Buffer gpuBuffer;
    uint32_t flags;
    uint32_t magicNumber;
  };

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
                        const int elementSize,
                        const BUFFER_FLAGS flags) override;

  void free(BufferHandle handle) override;

  void *getMappedData(const BufferHandle handle) const override;

  const vk::Buffer &getBufferData(const BufferHandle handle) const {
    assertMagicNumber(handle);
    uint32_t idx = getIndexFromHandle(handle);
    const VkBufferInfo &data = m_bufferStorage.getConstRef(idx);
    bool isGPUOnly = (data.flags & BUFFER_FLAGS_BITS::GPU_ONLY) > 0;
    return isGPUOnly ? data.gpuBuffer : data.cpuBuffer;
  }

  [[nodiscard]] VkBuffer getNativeBuffer(const BufferHandle &handle) const {
    assertMagicNumber(handle);
    uint32_t idx = getIndexFromHandle(handle);
    const VkBufferInfo &data = m_bufferStorage.getConstRef(idx);
    bool isGPUOnly = (data.flags & BUFFER_FLAGS_BITS::GPU_ONLY) > 0;
    return isGPUOnly ? data.gpuBuffer.buffer : data.cpuBuffer.buffer;
  }

  void bindBuffer(BufferHandle handle, VkWriteDescriptorSet *write,
                  VkDescriptorSet set, uint32_t bindingIndex) const;
  void update(BufferHandle handle, void *data, int offset, int size);

 private:
  inline void assertMagicNumber(const BufferHandle handle) const {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_bufferStorage.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }
  vk::Buffer allocateGPUVisibleBuffer(uint32_t sizeInBytes,
                                      VkBufferUsageFlags usage,
                                      const char *name, void *initData);
  vk::Buffer allocateCPUVisibleBuffer(uint32_t sizeInBytes,
                                      VkBufferUsageFlags usage,
                                      const char *name, void *initData);

 public:
  void transitionBuffer(const BufferHandle handle,
                        const BufferTransition &transition) override;

 private:
  SparseMemoryPool<VkBufferInfo> m_bufferStorage;
  static const uint32_t RESERVE_SIZE = 400;
  static const uint32_t RANDOM_ALLOC_RESERVE = 40 * 1024 * 1024;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  RandomSizeAllocator m_randomAlloc;
};

}  // namespace SirEngine::vk
