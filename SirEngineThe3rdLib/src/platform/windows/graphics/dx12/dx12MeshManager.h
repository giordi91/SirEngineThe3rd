#pragma once

#include "DXTK12/ResourceUploadBatch.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/meshManager.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <vector>

namespace SirEngine {
namespace dx12 {

struct MeshRuntime final {
  D3D12_VERTEX_BUFFER_VIEW vview;
  D3D12_INDEX_BUFFER_VIEW iview;
  uint32_t indexCount;
};

class Dx12MeshManager final : public MeshManager {
private:
  struct MeshData final {
    uint32_t magicNumber : 16;
    uint32_t stride : 16;
    ID3D12Resource *vertexBuffer;
    ID3D12Resource *indexBuffer;
    BufferHandle vtxBuffHandle;
    BufferHandle idxBuffHandle;
    MeshRuntime meshRuntime;
    uint32_t indexCount;
    uint32_t vertexCount;
    uint32_t entityID; // this is an id that is used to index other data that we
                       // are starting to split, for example bounding box;
  };

  struct MeshUploadResource final {
    ID3D12Resource *uploadVertexBuffer = nullptr;
    ID3D12Resource *uploadIndexBuffer = nullptr;
    UINT64 fence = 0;
  };

public:
  Dx12MeshManager() : m_meshPool(RESERVE_SIZE), batch(dx12::DEVICE) {
    m_nameToHandle.reserve(RESERVE_SIZE);
    m_uploadRequests.reserve(RESERVE_SIZE);
  }
  virtual ~Dx12MeshManager() { // assert(m_meshPool.assertEverythingDealloc());
  }

  inline uint32_t getIndexCount(const MeshHandle &handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);
    return data.indexCount;
  }
  Dx12MeshManager(const Dx12MeshManager &) = delete;
  Dx12MeshManager &operator=(const Dx12MeshManager &) = delete;
  // for now a bit overkill to pass both the index and the memory,
  // I could just pass the pointer at right address but for the time
  // being this will keep symmetry.

  // TODO fix is internal
  MeshHandle loadMesh(const char *path, bool isInternal = false) override;

  inline void assertMagicNumber(const MeshHandle handle) const {
#ifdef SE_DEBUG
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_meshPool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for constant buffer");
#endif
  }
  inline uint32_t getIndexFromHandle(MeshHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(MeshHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }

  DescriptorPair getSRVVertexBuffer(MeshHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);
    DescriptorPair pair;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferSRV(
        pair, data.vertexBuffer, data.vertexCount, data.stride);
    return pair;
  }
  DescriptorPair getSRVIndexBuffer(MeshHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);
    DescriptorPair pair;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferSRV(
        pair, data.indexBuffer, data.indexCount, sizeof(int));
    return pair;
  }
  inline D3D12_VERTEX_BUFFER_VIEW getVertexBufferView(MeshHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = data.vertexBuffer->GetGPUVirtualAddress();
    vbv.StrideInBytes = data.stride * sizeof(float);
    vbv.SizeInBytes = vbv.StrideInBytes * (data.vertexCount);
    return vbv;
  }

  inline D3D12_INDEX_BUFFER_VIEW getIndexBufferView(MeshHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);

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
    MeshData &data = m_meshPool[index];
    // releasing the texture;
    data.vertexBuffer->Release();
    data.indexBuffer->Release();
    // invalidating magic number
    data.magicNumber = 0;
    // adding the index to the free list
    m_meshPool.free(index);
  }

  const BoundingBox *getBoundingBoxes(uint32_t &outSize) const override {
    outSize = static_cast<uint32_t>(m_boundingBoxes.size());
    return m_boundingBoxes.data();
  }

  inline void bindMeshForRender(const MeshHandle handle,
                                FrameCommand *fc) const {
    auto vview = getVertexBufferView(handle);
    auto iview = getIndexBufferView(handle);
    fc->commandList->IASetIndexBuffer(&iview);
    fc->commandList->IASetVertexBuffers(0, 1, &vview);
    fc->commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  }
  inline void bindMeshRuntimeForRender(const MeshRuntime &runtime,
                                       FrameCommand *fc) const {
    fc->commandList->IASetIndexBuffer(&runtime.iview);
    fc->commandList->IASetVertexBuffers(0, 1, &runtime.vview);
    fc->commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  }
  inline void bindMeshAndRender(const MeshHandle handle,
                                FrameCommand *fc) const {
    bindMeshForRender(handle, fc);
    uint32_t meshIndexCount = getIndexCount(handle);

    fc->commandList->DrawIndexedInstanced(meshIndexCount, 1, 0, 0, 0);
  }

  [[nodiscard]] const MeshRuntime &
  getMeshRuntime(const MeshHandle &handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);
    return data.meshRuntime;
  }

  inline void bindMeshRuntimeAndRender(const MeshRuntime &runtime,
                                       FrameCommand *fc) const {
    bindMeshRuntimeForRender(runtime, fc);
    fc->commandList->DrawIndexedInstanced(runtime.indexCount, 1, 0, 0, 0);
  }

  inline void bindMeshRuntimeAndRender(const MeshHandle &handle,
                                       FrameCommand *fc) const {

    const MeshRuntime &runtime = getMeshRuntime(handle);
    bindMeshRuntimeAndRender(runtime, fc);
  }

private:
  SparseMemoryPool<MeshData> m_meshPool;

  std::unordered_map<std::string, MeshHandle> m_nameToHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  DirectX::ResourceUploadBatch batch;
  std::vector<MeshUploadResource> m_uploadRequests;
  std::vector<BoundingBox> m_boundingBoxes;
};

} // namespace dx12
} // namespace SirEngine
