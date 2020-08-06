#pragma once
#include "SirEngine/memory/cpu/SparseMemoryPool.h"
#include "SirEngine/memory/cpu/linearBufferManager.h"
#include "SirEngine/memory/gpu/gpuSlabAllocator.h"
//#include "platform/windows/graphics/vk/vkMemory.h"

namespace SirEngine::vk {
class VKGPUSlabAllocator final : GPUSlabAllocator {
  struct Slab {
    explicit Slab(const uint32_t sizeInByte) : m_slabTracker(sizeInByte){};
    LinearBufferManager m_slabTracker;
    //vk::Buffer m_buffer{};
    BufferHandle handle{};
  };

  struct SlabAllocData final {
    BufferRangeHandle m_rangeHandle;
    BufferRange m_range;
    uint32_t m_version : 16;
    uint32_t m_slabIdx : 16;
  };

 public:
  VKGPUSlabAllocator()
      : m_slabs(SLAB_RESERVE_SIZE), m_allocInfoStorage(POOL_RESERVE_SIZE){};
  virtual ~VKGPUSlabAllocator() = default;
  void initialize(GPUSlabAllocatorInitializeConfig config) override;
  GPUSlabAllocationHandle allocate(uint32_t sizeInBytes,
                                   void *initialData) override;
  void clear() override;
  void *getMappedPtr(GPUSlabAllocationHandle allocHandle) override;
  void cleanup() override;
  BufferHandle getBufferHandle(const uint32_t slabIndex) const {
    return m_slabs[slabIndex]->handle;
  }
  uint32_t getAllocatedBytes(const uint32_t slabIndex) const override {
    const auto *allocs =
        m_slabs.getConstRef(slabIndex)->m_slabTracker.getAllocations();
    int count = allocs->size();
    uint32_t total = 0;
    for (int i = 0; i < count; ++i) {
      const auto &alloc = allocs->getConstRef(i);
      total += alloc.m_range.m_size;
    }
    return total;
  };
  uint32_t getSlabCount() const override { return m_slabs.size(); };

  // not copiable/movable
  VKGPUSlabAllocator(const VKGPUSlabAllocator &) = delete;
  VKGPUSlabAllocator &operator=(const VKGPUSlabAllocator &) = delete;
  VKGPUSlabAllocator(VKGPUSlabAllocator &&o) noexcept =
      delete;  // move constructor
  VKGPUSlabAllocator &operator=(VKGPUSlabAllocator &&other) = delete;

 private:
  uint32_t allocateSlab();
  int getFreeSlabIndex(const uint32_t allocSize);

 private:
  GPUSlabAllocatorInitializeConfig m_config{};
  ResizableVector<Slab *> m_slabs;
  static constexpr uint32_t SLAB_RESERVE_SIZE = 16;
  static constexpr uint32_t POOL_RESERVE_SIZE = 16;
  uint32_t m_requireAlignment = 0;
  SparseMemoryPool<SlabAllocData> m_allocInfoStorage;
  uint32_t VERSION_COUNTER = 1;
};
}  // namespace SirEngine::vk
