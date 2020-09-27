#pragma once

#include <d3d12.h>

#include "SirEngine/bufferManager.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/cpu/SparseMemoryPool.h"
#include "SirEngine/memory/cpu/resizableVector.h"
#include "SirEngine/memory/cpu/stringHashMap.h"

namespace SirEngine::dx12 {
struct DescriptorPair;

class BufferManagerDx12 final : public BufferManager {
  struct UploadRequest {
    ID3D12Resource *uploadBuffer;
    uint64_t fence;
  };

 public:
  BufferManagerDx12()
      : m_bufferPool(RESERVE_SIZE), m_uploadRequests(RESERVE_SIZE) {}

  ~BufferManagerDx12() override = default;
  ID3D12Resource *getNativeBuffer(const BufferHandle bufferHandle);
  BufferManagerDx12(const BufferManagerDx12 &) = delete;
  BufferManagerDx12 &operator=(const BufferManagerDx12 &) = delete;

  // interface
  void free(const BufferHandle handle) override;
  void initialize() override{};
  void cleanup() override{};
  BufferHandle allocate(const uint32_t sizeInByte, void *initData,
                        const char *name, int numElements, int elementSize,
                        BUFFER_FLAGS flags) override;
  void *getMappedData(const BufferHandle handle) const override;

  void createUav(const BufferHandle &buffer, DescriptorPair &descriptor,
                 int offset, bool descriptorExits);
  void createSrv(const BufferHandle &handle, DescriptorPair &descriptorPair,
                 uint32_t offset = 0, bool descriptorExits = false) const;
  void createSrv(const BufferHandle &handle, DescriptorPair &descriptorPair,
                 MemoryRange range, bool descriptorExists = false,
                 int elementSize = -1) const;

  void clearUploadRequests();
  void update(BufferHandle handle, void *inData, int offset, int size) override;

 private:
  ID3D12Resource *allocateCpuVisibleBuffer(uint32_t actualSize) const;
  ID3D12Resource *allocateGpuVisibleBuffer(uint32_t actualSize, bool isUav);
  void uploadDataToGpuOnlyBuffer(void *initData, const uint32_t actualSize,
                                 const uint32_t offset, const bool isTemporary,
                                 ID3D12Resource *uploadBuffer,
                                 ID3D12Resource *buffer);

  inline void assertMagicNumber(const BufferHandle handle) const {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_bufferPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for constant buffer");
  }

 public:
  void transitionBuffer(const BufferHandle handle,
                        const BufferTransition &transition) override;

 private:
  struct BufferData {
    ID3D12Resource *data = nullptr;
    ID3D12Resource *uploadBuffer = nullptr;
    void *mappedData = nullptr;
    D3D12_RESOURCE_STATES state;
    uint32_t magicNumber;
    uint32_t elementCount;
    uint32_t elementSize;
    uint32_t flags;
  };

  static const int RESERVE_SIZE = 200;
  SparseMemoryPool<BufferData> m_bufferPool;
  ResizableVector<UploadRequest> m_uploadRequests;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};
}  // namespace SirEngine::dx12
