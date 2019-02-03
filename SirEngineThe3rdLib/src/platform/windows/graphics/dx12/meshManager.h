#pragma once

#include "DXTK12/ResourceUploadBatch.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <vector>

namespace SirEngine {
namespace dx12 {

struct MeshHandle final {
  uint32_t handle;
};

class MeshManager final {
private:
  struct MeshData final {
    uint32_t magicNumber : 16;
    uint32_t stride : 16;
    ID3D12Resource *vertexBuffer;
    ID3D12Resource *indexBuffer;
    uint32_t indexCount;
    uint32_t vertexCount;
  };

  struct MeshUploadResource final {
    ID3D12Resource *uploadVertexBuffer = nullptr;
    ID3D12Resource *uploadIndexBuffer = nullptr;
    UINT64 fence = 0;
  };

public:
  MeshManager() : batch(dx12::DEVICE) {
    m_staticStorage.reserve(RESERVE_SIZE);
    m_nameToHandle.reserve(RESERVE_SIZE);
    m_freeSlots.resize(RESERVE_SIZE);
	m_uploadRequests.reserve(RESERVE_SIZE);
  }
  ~MeshManager() = default;
  inline uint32_t getIndexCount(const MeshHandle &handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_staticStorage[index];
    return data.indexCount;
  }
  MeshManager(const MeshManager &) = delete;
  MeshManager &operator=(const MeshManager &) = delete;
  MeshHandle loadMesh(const char *path);

  inline void assertMagicNumber(MeshHandle handle) const {
#ifdef SE_DEBUG
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_staticStorage[idx].magicNumber == magic &&
           "invalid magic handle for constant buffer");
#endif
  }
  inline uint32_t getIndexFromHandle(MeshHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(MeshHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }

  DescriptorPair getSRVVertexBuffer(MeshHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_staticStorage[index];
    DescriptorPair pair;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferSRV(
        pair, data.vertexBuffer, data.vertexCount, data.stride);
    return pair;
  }
  DescriptorPair getSRVIndexBuffer(MeshHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_staticStorage[index];
    DescriptorPair pair;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferSRV(
        pair, data.indexBuffer, data.indexCount, sizeof(int));
    return pair;
  }
  inline D3D12_VERTEX_BUFFER_VIEW getVertexBufferView(MeshHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_staticStorage[index];

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = data.vertexBuffer->GetGPUVirtualAddress();
    vbv.StrideInBytes = data.stride * sizeof(float);
    vbv.SizeInBytes = vbv.StrideInBytes * (data.vertexCount);
    return vbv;
  }

  inline D3D12_INDEX_BUFFER_VIEW getIndexBufferView(MeshHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_staticStorage[index];

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = data.indexBuffer->GetGPUVirtualAddress();
    ibv.Format = DXGI_FORMAT_R32_UINT;
    ibv.SizeInBytes = data.indexCount * sizeof(int);
    return ibv;
  }

  void freeSRV(MeshHandle handle, DescriptorPair pair) const {
    assertMagicNumber(handle);
    assert(pair.type == DescriptorType::SRV);
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(pair);
  }
  inline MeshHandle getHandleFromName(const char *name) {
    auto found = m_nameToHandle.find(name);
    if (found != m_nameToHandle.end()) {
      return found->second;
    }
    return MeshHandle{0};
  }

  void free(const MeshHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    MeshData &data = m_staticStorage[index];
    // releasing the texture;
    data.vertexBuffer->Release();
    data.indexBuffer->Release();
    // invalidating magic number
    data.magicNumber = 0;
    // adding the index to the free list
    m_freeSlots[m_freeSlotIndex++] = index;
  }

  void clearUploadRequests();

private:
  MeshData &getFreeMeshData(uint32_t &index);

  std::vector<MeshData> m_staticStorage;
  std::unordered_map<std::string, MeshHandle> m_nameToHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  DirectX::ResourceUploadBatch batch;
  std::vector<uint32_t> m_freeSlots;
  uint32_t m_freeSlotIndex = 0;
  std::vector<MeshUploadResource> m_uploadRequests;
};

} // namespace dx12
} // namespace SirEngine
