#pragma once

#include <unordered_map>
#include <vector>

#include "SirEngine/constantBufferManager.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/cpu/linearBufferManager.h"
#include "SirEngine/memory/cpu/randomSizeAllocator.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"

// TODO this include is only really needed because we need Buffer definition
// which is overkill we want to investigate whether we can turn the constant
// buffer manager into a generic allocator on top of alower level allocator, that
// will allow us to remove the difference between dx12 and vk
#include "platform/windows/graphics/vk/vkBufferManager.h"

namespace SirEngine::vk {

class VkConstantBufferManager final : public ConstantBufferManager {
  struct Slab {
    Slab() : m_slabTracker(SLAB_ALLOCATION_IN_MB * MB_TO_BYTE){};
    LinearBufferManager m_slabTracker;
    vk::Buffer m_buffer{};
  };

 public:
  static constexpr uint64_t SLAB_ALLOCATION_IN_MB = 32;
  static constexpr uint64_t MAX_ALLOCATED_SLABS = 10;

 public:
  VkConstantBufferManager() : m_allocInfoStorage(RESERVE_SIZE) {}
  virtual ~VkConstantBufferManager() = default;
  void initialize() override;
  void cleanup() override;
  void clearUpQueueFree();
  // deleted method to avoid copy, you can still move it though
  VkConstantBufferManager(const VkConstantBufferManager &) = delete;
  VkConstantBufferManager &operator=(const VkConstantBufferManager &) = delete;

  ConstantBufferHandle allocate(uint32_t sizeInBytes,
                                CONSTANT_BUFFER_FLAGS flags = 0,
                                void *data = nullptr) override;
  void update(ConstantBufferHandle handle, void *data) override;

  bool free(ConstantBufferHandle handle) override;

  // this function should be called at the beginning of the frame, if there is
  // any buffered constant buffer will be dealt with
  virtual void processBufferedData() override;

  // vk methods
  void bindConstantBuffer(ConstantBufferHandle handle,
                          VkDescriptorBufferInfo &bufferInfo,
                          uint32_t bindingIdx, VkWriteDescriptorSet *set,
                          VkDescriptorSet descSet) const;
  inline VkDescriptorBufferInfo getBufferDescriptor(
      const ConstantBufferHandle &handle) const {
    assertMagicNumber(handle);
    uint32_t idx = getIndexFromHandle(handle);
    const ConstantBufferData &buffData = m_allocInfoStorage.getConstRef(idx);
    const Slab &slab =
        m_perFrameSlabs[globals::CURRENT_FRAME][buffData.m_slabIdx];

    // actual information of the descriptor, in this case it is our mesh buffer
    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = slab.m_buffer.buffer;
    bufferInfo.offset = buffData.m_range.m_offset;
    bufferInfo.range = buffData.m_range.m_size;
    return bufferInfo;
  }

  [[nodiscard]] const ResizableVector<BufferRangeTracker> *getAllocations()
      const;

 private:
  // struct ConstantBufferedData {
  //  ConstantBufferHandle handle;
  //  RandomSizeAllocationHandle dataAllocHandle;
  //  int counter = 0;
  //  int poolIndex;
  //};
  // struct ConstantBufferDataDynamic {
  //  ConstantBufferData cbData[FRAME_BUFFERS_COUNT];
  //};

  struct ConstantBufferData final {
    BufferRangeHandle m_rangeHandle;
    BufferRange m_range;
    uint32_t m_flags;
    uint32_t m_magicNumber : 16;
    uint32_t m_slabIdx : 16;
  };

  struct ConstantBufferedData {
    ConstantBufferHandle handle;
    // this data represent the data we need to copy in the buffer
    // this data is on the CPU and will be used for the memcpy
    RandomSizeAllocationHandle dataAllocHandle;
    int counter = 0;
  };

 private:
  void allocateSlab();
  int getFreeSlabIndex(uint32_t allocSize);

  inline void assertMagicNumber(const ConstantBufferHandle handle) const {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_allocInfoStorage.getConstRef(idx).m_magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }

 private:
  SparseMemoryPool<ConstantBufferData> m_allocInfoStorage;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 400;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  RandomSizeAllocator m_randomAlloc;
  std::unordered_map<uint32_t, ConstantBufferedData> m_bufferedRequests;
  std::vector<uint32_t> m_bufferToFree;

  Slab *m_slabs = nullptr;
  Slab **m_perFrameSlabs = nullptr;
  uint32_t m_allocatedSlabs = 0;
  uint32_t m_requireAlignment = 0;
};

}  // namespace SirEngine::vk
