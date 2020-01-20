#pragma once

#include <unordered_map>

#include "SirEngine/bufferManager.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "d3dx12.h"
#include "descriptorHeap.h"

namespace SirEngine {
namespace dx12 {
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
  void free(const BufferHandle handle) override;
  // this function only exists to have api consistency

  void initialize() override{};
  void cleanup() override{};

  BufferHandle allocate(const uint32_t sizeInByte, void *initData,
                        const char *name, int numElements, int elementSize,
                        uint32_t flags) override;
  BufferHandle allocateUpload(const uint32_t sizeInByte,
                              const uint32_t numElements,
                              const uint32_t elementSize,
                              const char *name) override;

  void bindBuffer(BufferHandle handle, int slot,
                  ID3D12GraphicsCommandList2 *commandList) const;
  void bindBufferAsSRVGraphics(BufferHandle handle, int slot,
                               ID3D12GraphicsCommandList2 *commandList,
                               uint32_t offset = 0) const;
  void createSrv(const BufferHandle &handle, DescriptorPair &descriptorPair,
                 uint32_t offset = 0) const;
  void
  bindBufferAsDescriptorTableGrahpics(const BufferHandle handle, const int slot,
                                      ID3D12GraphicsCommandList2 *commandList,
                                      uint32_t offset) const;

  BufferHandle getBufferFromName(const std::string &name) const {
    const auto found = m_nameToHandle.find(name);
    if (found != m_nameToHandle.end()) {
      return found->second;
    }

    return BufferHandle{0};
  }

  void *getMappedData(const BufferHandle handle) const override;

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
  inline int transitionBufferIfNeeded(const BufferHandle handle,
                                      const D3D12_RESOURCE_STATES wantedState,
                                      D3D12_RESOURCE_BARRIER *barriers,
                                      int counter) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    BufferData &data = m_bufferPool[index];

    auto state = data.state;
    if (state != wantedState) {
      barriers[counter] =
          CD3DX12_RESOURCE_BARRIER::Transition(data.data, state, wantedState);
      data.state = wantedState;
      ++counter;
    }
    return counter;
  }
  void clearUploadRequests();

  // inline BufferHandle getHandleFromName(const char *name) {
  //  auto found = m_nameToHandle.find(name);
  //  if (found != m_nameToHandle.end()) {
  //    return found->second;
  //  }
  //  return BufferHandle{0};
  //}
private:
  inline void assertMagicNumber(const BufferHandle handle) const {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_bufferPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for constant buffer");
  }
  enum class BufferType { UAV, SRV };
  struct BufferData {
    ID3D12Resource *data;
    BufferType type;
    D3D12_RESOURCE_STATES state;
    uint32_t magicNumber;
    DescriptorPair srv;
    DescriptorPair uav;
    uint32_t elementCount;
    uint32_t elementSize;
    void *mappedData = nullptr;
  };

  static const int RESERVE_SIZE = 200;
  SparseMemoryPool<BufferData> m_bufferPool;
  std::unordered_map<std::string, BufferHandle> m_nameToHandle;
  std::vector<UploadRequest> m_uploadRequests;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};
} // namespace dx12
} // namespace SirEngine
