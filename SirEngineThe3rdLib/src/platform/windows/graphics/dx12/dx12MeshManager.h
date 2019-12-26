#pragma once

#include "DXTK12/ResourceUploadBatch.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/meshManager.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/dx12BufferManager.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <vector>

namespace SirEngine {
namespace dx12 {

struct MeshRuntime final {
  D3D12_INDEX_BUFFER_VIEW iview;
  uint32_t indexCount;
  BufferHandle bufferHandle;
  MemoryRange positionRange;
  MemoryRange normalsRange;
  MemoryRange uvRange;
  MemoryRange tangentsRange;
};

class Dx12MeshManager final : public MeshManager {
private:
  struct MeshData final {
    uint32_t magicNumber : 16;
    uint32_t stride : 16;
    ID3D12Resource *indexBuffer;
    BufferHandle idxBuffHandle;
    MeshRuntime meshRuntime;
    uint32_t indexCount;
    uint32_t vertexCount;
    uint32_t entityID; // this is an id that is used to index other data that we
                       // are starting to split, for example bounding box;
  };

public:
  Dx12MeshManager() : m_meshPool(RESERVE_SIZE), batch(dx12::DEVICE) {
    m_nameToHandle.reserve(RESERVE_SIZE);
  }
  virtual ~Dx12MeshManager() { // assert(m_meshPool.assertEverythingDealloc());
  }

  Dx12MeshManager(const Dx12MeshManager &) = delete;
  Dx12MeshManager &operator=(const Dx12MeshManager &) = delete;

  // TODO fix is internal
  MeshHandle loadMesh(const char *path, bool isInternal = false) override;

  DescriptorPair getSRVIndexBuffer(MeshHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);
    DescriptorPair pair;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createBufferSRV(
        pair, data.indexBuffer, data.indexCount, sizeof(int));
    return pair;
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

  inline void bindMeshRuntimeForRender(const MeshRuntime &runtime,
                                       FrameCommand *fc) const {
    fc->commandList->IASetIndexBuffer(&runtime.iview);
    fc->commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  }

  [[nodiscard]] const MeshRuntime &
  getMeshRuntime(const MeshHandle &handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);
    return data.meshRuntime;
  }
  void bindMeshRuntimeAndRenderPosOnly(const MeshRuntime &meshRuntime,
                                       FrameCommand *currentFc) {
    bindMeshRuntimeForRender(meshRuntime, currentFc);

    dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(
        meshRuntime.bufferHandle, 4, currentFc->commandList,
        meshRuntime.positionRange.m_offset);
  }
  void render(const MeshRuntime &meshRuntime, FrameCommand *currentFc) {
    currentFc->commandList->DrawIndexedInstanced(meshRuntime.indexCount, 1, 0,
                                                 0, 0);
  }
  void render(const MeshHandle handle, FrameCommand *currentFc) {
    const MeshRuntime &runtime = getMeshRuntime(handle);
    currentFc->commandList->DrawIndexedInstanced(runtime.indexCount, 1, 0,
                                                 0, 0);
  }

  inline void bindMesh(MeshHandle handle, ID3D12GraphicsCommandList2 *commandList, uint32_t flags,
                       uint32_t startIndex) {
    const MeshRuntime &runtime = getMeshRuntime(handle);
    if ((flags & MeshAttributeFlags::POSITIONS) > 0) {
      dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(
          runtime.bufferHandle, startIndex + 0, commandList,
          runtime.positionRange.m_offset);
    }
    if ((flags & MeshAttributeFlags::NORMALS) > 0) {
      dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(
          runtime.bufferHandle, startIndex + 1, commandList,
          runtime.normalsRange.m_offset);
    }
    if ((flags & MeshAttributeFlags::UV) > 0) {
      dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(
          runtime.bufferHandle, startIndex + 2, commandList,
          runtime.uvRange.m_offset);
    }
    if ((flags & MeshAttributeFlags::TANGENTS) > 0) {
      dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(
          runtime.bufferHandle, startIndex + 3, commandList,
          runtime.tangentsRange.m_offset);
    }
    commandList->IASetIndexBuffer(&runtime.iview);
    commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  }

  inline void bindMeshRuntimeAndRender(const MeshRuntime &runtime,
                                       FrameCommand *fc) const {

    dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(
        runtime.bufferHandle, 9, fc->commandList,
        runtime.positionRange.m_offset);
    dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(
        runtime.bufferHandle, 10, fc->commandList,
        runtime.normalsRange.m_offset);
    dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(
        runtime.bufferHandle, 11, fc->commandList, runtime.uvRange.m_offset);
    dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(
        runtime.bufferHandle, 12, fc->commandList,
        runtime.tangentsRange.m_offset);
    fc->commandList->DrawIndexedInstanced(runtime.indexCount, 1, 0, 0, 0);
  }

  inline void bindMeshRuntimeAndRender(const MeshHandle &handle,
                                       FrameCommand *fc) const {

    const MeshRuntime &runtime = getMeshRuntime(handle);
    bindMeshRuntimeAndRender(runtime, fc);
  }

private:
  inline void assertMagicNumber(const MeshHandle handle) const {
#ifdef SE_DEBUG
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_meshPool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for constant buffer");
#endif
  }

private:
  SparseMemoryPool<MeshData> m_meshPool;

  std::unordered_map<std::string, MeshHandle> m_nameToHandle;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  DirectX::ResourceUploadBatch batch;
  std::vector<BoundingBox> m_boundingBoxes;
};

} // namespace dx12
} // namespace SirEngine
