#pragma once

#include <unordered_map>

#include "SirEngine/bufferManager.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/cpu/SparseMemoryPool.h"
#include "d3dx12.h"
#include "descriptorHeap.h"

namespace SirEngine::dx12 {
class BufferManagerDx12 final : public BufferManager {
  struct UploadRequest {
    ID3D12Resource *uploadBuffer;
    uint64_t fence;
  };

 public:
  BufferManagerDx12() : m_bufferPool(RESERVE_SIZE){};

  virtual ~BufferManagerDx12() = default;
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

  // dx12 specific functions
  void bindBufferAsSRVGraphics(BufferHandle handle, int slot,
                               ID3D12GraphicsCommandList2 *commandList,
                               uint32_t offset = 0) const;
  void createSrv(const BufferHandle &handle, DescriptorPair &descriptorPair,
                 uint32_t offset = 0, bool descriptorExits = false) const;
  void createSrv(const BufferHandle &handle, DescriptorPair &descriptorPair,
                 MemoryRange range, bool descriptorExists = false,
                 int elementSize = -1) const;


  inline int bufferUAVTransition(const BufferHandle handle,
                                 D3D12_RESOURCE_BARRIER *barriers,
                                 int counter) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const BufferData &data = m_bufferPool.getConstRef(index);
    barriers[counter].UAV.pResource = data.data;
    barriers[counter].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    ++counter;
    return counter;
  }

  void clearUploadRequests();

 private:
  ID3D12Resource *allocateCpuVisibleBuffer(uint32_t actualSize) const;
  ID3D12Resource *allocateGpuVisibleBuffer(uint32_t actualSize, bool isUav);
  void uploadDataToGpuOnlyBuffer(void *initData, uint32_t actualSize,
                                 bool isTemporary, ID3D12Resource *uploadBuffer,
                                 ID3D12Resource *buffer);

  inline void assertMagicNumber(const BufferHandle handle) const {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_bufferPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for constant buffer");
  }

  struct BufferData {
    ID3D12Resource *data = nullptr;
    void *mappedData = nullptr;
    D3D12_RESOURCE_STATES state;
    uint32_t magicNumber;
    uint32_t elementCount;
    uint32_t elementSize;
    uint32_t flags;
  };

  static const int RESERVE_SIZE = 200;
  SparseMemoryPool<BufferData> m_bufferPool;
  std::unordered_map<std::string, BufferHandle> m_nameToHandle;
  std::vector<UploadRequest> m_uploadRequests;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};
}  // namespace SirEngine::dx12
