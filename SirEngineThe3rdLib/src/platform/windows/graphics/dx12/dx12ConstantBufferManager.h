#pragma once
#include "SirEngine/constantBufferManager.h"
#include "SirEngine/globals.h"
#include "SirEngine/handle.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/SparseMemoryPool.h"
#include "SirEngine/memory/randomSizeAllocator.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include <vector>

namespace SirEngine::dx12 {

class Dx12ConstantBufferManager final : public ConstantBufferManager {
public:
  Dx12ConstantBufferManager() : m_dynamicStorage(RESERVE_SIZE) {}
  virtual ~Dx12ConstantBufferManager() = default;
  void initialize() override;
  void cleanup() override;
  void clearUpQueueFree();
  // deleted method to avoid copy, you can still move it though
  Dx12ConstantBufferManager(const Dx12ConstantBufferManager &) = delete;
  Dx12ConstantBufferManager &
  operator=(const Dx12ConstantBufferManager &) = delete;

  ConstantBufferHandle allocateDynamic(uint32_t sizeInBytes,
                                       void *data = nullptr) override;
  ConstantBufferHandle allocate(uint32_t sizeInBytes, uint32_t flags = 0,
                                void *data = nullptr) override {
    assert(0);
    return {};
  };
  void update(ConstantBufferHandle handle, void *data) override { assert(0); }

  bool free(ConstantBufferHandle handle) override;

  inline DescriptorPair
  getConstantBufferDx12Handle(const ConstantBufferHandle handle) {

    // making sure the resource has not been de-allocated
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    return m_dynamicStorage[index].cbData[globals::CURRENT_FRAME].pair;
  }

  inline D3D12_GPU_VIRTUAL_ADDRESS
  getVirtualAddress(const ConstantBufferHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_dynamicStorage[index]
        .cbData[globals::CURRENT_FRAME]
        .resource->GetGPUVirtualAddress();
  }

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
    ID3D12Resource *resource = nullptr;
    uint64_t fence = 0;
    bool mapped : 1;
    uint32_t size : 31;
    uint32_t magicNumber : 16;
    uint32_t padding : 16;
    DescriptorPair pair;
  };

  struct ConstantBufferDataDynamic {
    ConstantBufferData cbData[FRAME_BUFFERS_COUNT];
  };

private:
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
      data.resource->Map(0, nullptr,
                         reinterpret_cast<void **>(&data.mappedData));
    } else {
      SE_CORE_WARN("Tried to map an already mapped buffer");
    }
  };
  inline void unmapConstantBuffer(ConstantBufferData &data) const {
    if (data.mapped) {
      data.mapped = false;
      data.resource->Unmap(0, nullptr);
    } else {
      SE_CORE_WARN("Tried to unmap an already mapped buffer");
    }
  }

private:
  ;

private:
  // std::vector<ConstantBufferData> m_dynamicStorage[FRAME_BUFFERS_COUNT];

  SparseMemoryPool<ConstantBufferDataDynamic> m_dynamicStorage;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 400;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  RandomSizeAllocator m_randomAlloc;
  std::unordered_map<uint32_t, ConstantBufferedData> m_bufferedRequests;
  std::vector<uint32_t> m_bufferToFree;
};

} // namespace SirEngine::dx12
