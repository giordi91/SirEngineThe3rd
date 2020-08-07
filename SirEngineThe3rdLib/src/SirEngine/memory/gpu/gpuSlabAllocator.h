#pragma once
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/cpu/SparseMemoryPool.h"
#include "SirEngine/memory/cpu/linearBufferManager.h"

namespace SirEngine {
struct GPUSlabAllocatorInitializeConfig {
  uint32_t slabSizeInBytes = 16 * MB_TO_BYTE;
  uint32_t initialSlabs = 1;
  bool allowNewSlabAllocations = true;
  uint32_t allocationsRequestAligment = 4;
};

class GPUSlabAllocator final {
  struct Slab {
    explicit Slab(const uint32_t sizeInByte) : m_slabTracker(sizeInByte){};
    LinearBufferManager m_slabTracker;
    // vk::Buffer m_buffer{};
    BufferHandle handle{};
  };

  struct SlabAllocData final {
    BufferRangeHandle m_rangeHandle;
    BufferRange m_range;
    uint32_t m_version : 16;
    uint32_t m_slabIdx : 16;
  };

 public:
  GPUSlabAllocator()
      : m_slabs(SLAB_RESERVE_SIZE), m_allocInfoStorage(POOL_RESERVE_SIZE){};
  virtual ~GPUSlabAllocator() = default;
  void initialize(GPUSlabAllocatorInitializeConfig config);
  GPUSlabAllocationHandle allocate(uint32_t sizeInBytes, void *initialData);
  void clear();
  void *getMappedPtr(GPUSlabAllocationHandle allocHandle);
  void cleanup();
  BufferHandle getBufferHandle(const uint32_t slabIndex) const {
    return m_slabs[slabIndex]->handle;
  }
  uint32_t getAllocatedBytes(const uint32_t slabIndex) const {
    const auto *allocs =
        m_slabs.getConstRef(slabIndex)->m_slabTracker.getAllocations();
    int count = allocs->size();
    uint64_t total = 0;
    for (int i = 0; i < count; ++i) {
      const auto &alloc = allocs->getConstRef(i);
      total += alloc.m_range.m_size;
    }
    return static_cast<uint32_t>(total);
  };
  uint32_t getSlabCount() const { return m_slabs.size(); };

  // not copiable/movable
  GPUSlabAllocator(const GPUSlabAllocator &) = delete;
  GPUSlabAllocator &operator=(const GPUSlabAllocator &) = delete;
  GPUSlabAllocator(GPUSlabAllocator &&o) noexcept = delete;  // move constructor
  GPUSlabAllocator &operator=(GPUSlabAllocator &&other) = delete;

 private:
  uint32_t allocateSlab();
  int getFreeSlabIndex(const uint32_t allocSize);

 private:
  GPUSlabAllocatorInitializeConfig m_config{};
  ResizableVector<Slab *> m_slabs;
  static constexpr uint32_t SLAB_RESERVE_SIZE = 16;
  static constexpr uint32_t POOL_RESERVE_SIZE = 16;
  SparseMemoryPool<SlabAllocData> m_allocInfoStorage;
  uint32_t VERSION_COUNTER = 1;
};
}  // namespace SirEngine
