#pragma once

#include "DXTK12/ResourceUploadBatch.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/cpu/resizableVector.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/memory/cpu/stringHashMap.h"
#include "SirEngine/meshManager.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/dx12BufferManager.h"

namespace SirEngine {
namespace dx12 {

struct Dx12MeshRuntime final {
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
    Dx12MeshRuntime meshRuntime;
    uint32_t indexCount;
    uint32_t vertexCount;
    uint32_t entityID;  // this is an id that is used to index other data that
                        // we are starting to split, for example bounding box;
  };

 public:
  Dx12MeshManager()
      : m_meshPool(RESERVE_SIZE),
        batch(dx12::DEVICE),
        m_boundingBoxes(RESERVE_SIZE),
        m_nameToHandle(RESERVE_SIZE) {}
  virtual ~Dx12MeshManager() override = default;

  void initialize() override{};
  void cleanup() override{};

  Dx12MeshManager(const Dx12MeshManager &) = delete;
  Dx12MeshManager &operator=(const Dx12MeshManager &) = delete;

  MeshHandle loadMesh(const char *path) override;

  MeshHandle getHandleFromName(const char *name) const override {
    MeshHandle handle{};
    m_nameToHandle.get(name,handle);
    assert(handle.isHandleValid());
    return handle;
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

  [[nodiscard]] const Dx12MeshRuntime &getMeshRuntime(
      const MeshHandle &handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);
    return data.meshRuntime;
  }
  static void render(const Dx12MeshRuntime &meshRuntime,
                     FrameCommand *currentFc, const bool indexed = true) {
    if (indexed) {
      currentFc->commandList->DrawIndexedInstanced(meshRuntime.indexCount, 1, 0,
                                                   0, 0);
    } else {
      currentFc->commandList->DrawInstanced(meshRuntime.indexCount, 1, 0, 0);
    }
  }
  void render(const MeshHandle handle, FrameCommand *currentFc) const {
    const Dx12MeshRuntime &runtime = getMeshRuntime(handle);
    currentFc->commandList->IASetIndexBuffer(&runtime.iview);
    currentFc->commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    currentFc->commandList->DrawIndexedInstanced(runtime.indexCount, 1, 0, 0,
                                                 0);
  }

  inline void bindFlatMesh(const MeshHandle handle, DescriptorPair *pairs,
                           const uint32_t flags,
                           const uint32_t startIndex) const {
    const Dx12MeshRuntime &runtime = getMeshRuntime(handle);
    if ((flags & MESH_ATTRIBUTE_FLAGS::POSITIONS) > 0) {
      // TODO The element size of the srv for now is hardcoded, this value is
      // defined by the compiler and probably should simply put in the mesh file
      // definition
      dx12::BUFFER_MANAGER->createSrv(
          runtime.bufferHandle, pairs[startIndex + 0], runtime.positionRange,
          true, sizeof(float));
    }
    if ((flags & MESH_ATTRIBUTE_FLAGS::NORMALS) > 0) {
      dx12::BUFFER_MANAGER->createSrv(
          runtime.bufferHandle, pairs[startIndex + 1], runtime.normalsRange,
          true, sizeof(float));
    }
    if ((flags & MESH_ATTRIBUTE_FLAGS::UV) > 0) {
      dx12::BUFFER_MANAGER->createSrv(runtime.bufferHandle,
                                      pairs[startIndex + 2], runtime.uvRange,
                                      true, sizeof(float));
    }
    if ((flags & MESH_ATTRIBUTE_FLAGS::TANGENTS) > 0) {
      dx12::BUFFER_MANAGER->createSrv(
          runtime.bufferHandle, pairs[startIndex + 3], runtime.tangentsRange,
          true, sizeof(float));
    }
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

  // change this unordered map
  HashMap<const char *, MeshHandle, hashString32> m_nameToHandle;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  DirectX::ResourceUploadBatch batch;
  ResizableVector<BoundingBox> m_boundingBoxes;
};

}  // namespace dx12
}  // namespace SirEngine
