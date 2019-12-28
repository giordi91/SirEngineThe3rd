#pragma once

#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/meshManager.h"
#include "platform/windows/graphics/vk/vkMemory.h"
#include "platform/windows/graphics/vk/volk.h"
#include <unordered_map>
#include <vector>

namespace SirEngine::vk {

struct VkMeshRuntime final {
  VkBuffer vertexBuffer;
  VkBuffer indexBuffer;
  uint32_t indexCount;
  MemoryRange positionRange;
  MemoryRange normalsRange;
  MemoryRange uvRange;
  MemoryRange tangentsRange;
};

struct MeshData final {
  uint32_t magicNumber : 16;
  uint32_t stride : 16;
  VkBuffer vertexBuffer;
  VkBuffer indexBuffer;
  BufferHandle vtxBuffHandle;
  BufferHandle idxBuffHandle;
  uint32_t indexCount;
  uint32_t vertexCount;
  uint32_t entityID; // this is an id that is used to index other data that we
                     // are starting to split, for example bounding box;
  VkMeshRuntime meshRuntime;
};
class VkMeshManager final : public MeshManager {
private:
  struct MeshUploadResource final {
    // ID3D12Resource *uploadVertexBuffer = nullptr;
    // ID3D12Resource *uploadIndexBuffer = nullptr;
    UINT64 fence = 0;
  };

public:
  VkMeshManager() : m_meshPool(RESERVE_SIZE) {
    m_nameToHandle.reserve(RESERVE_SIZE);
    m_uploadRequests.reserve(RESERVE_SIZE);
  }
  ~VkMeshManager() { // assert(m_meshPool.assertEverythingDealloc());
  }

  inline uint32_t getIndexCount(const MeshHandle &handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);
    return data.indexCount;
  }

  vk::Buffer getVertexBuffer(const MeshHandle &handle);
  vk::Buffer getIndexBuffer(const MeshHandle &handle);
  const MeshData &getMeshData(const MeshHandle &handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_meshPool.getConstRef(index);
  }

  VkMeshManager(const VkMeshManager &) = delete;
  VkMeshManager &operator=(const VkMeshManager &) = delete;
  // for now a bit overkill to pass both the index and the memory,
  // I could just pass the pointer at right address but for the time
  // being this will keep symmetry.
  MeshHandle loadMesh(const char *path, bool isInternal = false) override;
  const BoundingBox *getBoundingBoxes(uint32_t &outSize) const override {
    assert(0);
    return nullptr;
  };

  inline void renderMesh(const MeshHandle handle,
                         const VkCommandBuffer commandBuffer) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);

    vkCmdBindIndexBuffer(commandBuffer, data.indexBuffer, 0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, data.indexCount, 1, 0, 0, 0);
  };

  void free(const MeshHandle handle) override;
  // vk methods
  VkMeshRuntime getMeshRuntime(const MeshHandle &handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);
    return data.meshRuntime;
  };
  void bindMesh(MeshHandle handle, VkWriteDescriptorSet *set,
                VkDescriptorSet descriptorSet, VkDescriptorBufferInfo *info);

private:
  inline void assertMagicNumber(const MeshHandle handle) const {
#ifdef SE_DEBUG
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_meshPool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for constant buffer");
#endif
  }
  inline MeshHandle getHandleFromName(const char *name) {
    auto found = m_nameToHandle.find(name);
    if (found != m_nameToHandle.end()) {
      return found->second;
    }
    return MeshHandle{0};
  }

  inline const std::vector<BoundingBox> &getBoundingBoxes() {
    return m_boundingBoxes;
  }

  /*
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
  inline void bindMeshRuntimeAndRender(const MeshRuntime &runtime,
                                       FrameCommand *fc) const {
    bindMeshRuntimeForRender(runtime, fc);
    fc->commandList->DrawIndexedInstanced(runtime.indexCount, 1, 0, 0, 0);
  }
  */

private:
  SparseMemoryPool<MeshData> m_meshPool;

  std::unordered_map<std::string, MeshHandle> m_nameToHandle;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  std::vector<MeshUploadResource> m_uploadRequests;
  std::vector<BoundingBox> m_boundingBoxes;
};

} // namespace SirEngine::vk
