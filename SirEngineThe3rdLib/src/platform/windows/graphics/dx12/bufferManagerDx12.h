#pragma once

#include "SirEngine/bufferManager.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/SparseMemoryPool.h"
#include "d3dx12.h"
#include <unordered_map>
#include "descriptorHeap.h"

namespace SirEngine {
namespace dx12 {
class BufferManagerDx12 final : public BufferManager {
public:
  BufferManagerDx12():m_bufferPool(RESERVE_SIZE){};

  virtual ~BufferManagerDx12() = default;
  BufferManagerDx12(const BufferManagerDx12 &) = delete;
  BufferManagerDx12 &operator=(const BufferManagerDx12 &) = delete;
  void free(const BufferHandle handle) override;
  //this function only exists to have api consistency
  void initialize(){};

  BufferHandle allocate(const uint32_t sizeInByte, void *initData,
                        const char *name,int numElements, int elementSize, bool isUAV) override;

  void bindBuffer(BufferHandle handle, int slot, ID3D12GraphicsCommandList2 *commandList) const;

  // inline BufferHandle getHandleFromName(const char *name) {
  //  auto found = m_nameToHandle.find(name);
  //  if (found != m_nameToHandle.end()) {
  //    return found->second;
  //  }
  //  return BufferHandle{0};
  //}
private:
  inline uint32_t getIndexFromHandle(const BufferHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(const BufferHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }
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
  };

  static const int RESERVE_SIZE =200;
  SparseMemoryPool<BufferData> m_bufferPool;
};
} // namespace dx12
} // namespace SirEngine
