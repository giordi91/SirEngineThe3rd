#pragma once
#include "SirEngine/constantBufferManager.h"
#include "SirEngine/handle.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/randomSizeAllocator.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "platform/windows/graphics/vk/volk.h"
#include "vkMemory.h"
#include <unordered_map>
#include <vector>

namespace SirEngine::vk {

class VkConstantBufferManager final : public ConstantBufferManager {
public:
  static constexpr uint64_t SLAB_ALLOCATION_IN_MB = 256;
  static constexpr uint64_t MAX_ALLOCATED_SLABS= 10;

public:
  VkConstantBufferManager() : m_dynamicStorage(RESERVE_SIZE) {}
  virtual ~VkConstantBufferManager() = default;
  void initialize();
  void clearUpQueueFree();
  // deleted method to avoid copy, you can still move it though
  VkConstantBufferManager(const VkConstantBufferManager &) = delete;
  VkConstantBufferManager &operator=(const VkConstantBufferManager &) = delete;

  virtual ConstantBufferHandle allocateDynamic(uint32_t sizeInBytes,
                                               void *data = nullptr) override;

  ConstantBufferHandle allocate(uint32_t sizeInBytes, uint32_t flags = 0,
                                void *data = nullptr) override;

  bool free(ConstantBufferHandle handle) override;

  virtual void
  updateConstantBufferNotBuffered(const ConstantBufferHandle handle,
                                  void *dataToUpload) override;

  virtual void updateConstantBufferBuffered(const ConstantBufferHandle handle,
                                            void *dataToUpload) override;

  // this function should be called at the beginning of the frame, if there is
  // any buffered constant buffer will be dealt with
  virtual void processBufferedData() override;

private:
  struct ConstantBufferedData {
    ConstantBufferHandle handle;
    RandomSizeAllocationHandle dataAllocHandle;
    int counter = 0;
    int poolIndex;
  };

  struct ConstantBufferData final {
    // we are using one byte for the mapped flag and 31 bytes for the
    // the actual data size, we can't have buffers that big anyway
    uchar *mappedData = nullptr;
    // ID3D12Resource *resource = nullptr;
    uint64_t fence = 0;
    bool mapped : 1;
    uint32_t size : 31;
    uint32_t magicNumber : 16;
    uint32_t padding : 16;
    // DescriptorPair pair;
  };

  struct ConstantBufferDataDynamic {
    ConstantBufferData cbData[FRAME_BUFFERS_COUNT];
  };

private:
  void allocateSlab();

  inline void assertMagicNumber(const ConstantBufferHandle handle) {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_dynamicStorage[idx].cbData[globals::CURRENT_FRAME].magicNumber ==
               magic &&
           "invalid magic handle for constant buffer");
  }
  inline void mapConstantBuffer(ConstantBufferData &data) const {
    if (!data.mapped) {
      data.mapped = true;
      // data.resource->Map(0, nullptr,
      //                   reinterpret_cast<void **>(&data.mappedData));
    } else {
      SE_CORE_WARN("Tried to map an already mapped buffer");
    }
  };
  inline void unmapConstantBuffer(ConstantBufferData &data) const {
    if (data.mapped) {
      data.mapped = false;
      // data.resource->Unmap(0, nullptr);
    } else {
      SE_CORE_WARN("Tried to unmap an already mapped buffer");
    }
  };

private:
  SparseMemoryPool<ConstantBufferDataDynamic> m_dynamicStorage;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 400;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  RandomSizeAllocator m_randomAlloc;
  std::unordered_map<uint32_t, ConstantBufferedData> m_bufferedRequests;
  std::vector<uint32_t> m_bufferToFree;

  vk::Buffer slabs[MAX_ALLOCATED_SLABS];
  uint32_t m_allocatedSlabs = 0;
};

} // namespace SirEngine::vk
