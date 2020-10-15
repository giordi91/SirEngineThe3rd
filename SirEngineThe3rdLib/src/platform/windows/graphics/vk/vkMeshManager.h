#pragma once

#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/cpu/SparseMemoryPool.h"
#include "SirEngine/memory/cpu/resizableVector.h"
#include "SirEngine/memory/cpu/stringHashMap.h"
#include "SirEngine/meshManager.h"
#include "platform/windows/graphics/vk/volk.h"

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
  uint32_t entityID;  // this is an id that is used to index other data that we
                      // are starting to split, for example bounding box;
  VkMeshRuntime meshRuntime;
};
class VkMeshManager final : public MeshManager {
 private:
  struct MeshUploadResource final {
    uint64_t fence = 0;
  };

 public:
  VkMeshManager()
      : m_meshPool(RESERVE_SIZE),
        m_nameToHandle(RESERVE_SIZE),
        m_uploadRequests(RESERVE_SIZE),
        m_boundingBoxes(RESERVE_SIZE) {}
  ~VkMeshManager() override = default;

  void initialize() override{};
  void cleanup() override;
  inline uint32_t getIndexCount(const MeshHandle &handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);
    return data.indexCount;
  }

  VkMeshManager(const VkMeshManager &) = delete;
  VkMeshManager &operator=(const VkMeshManager &) = delete;
  // for now a bit overkill to pass both the index and the memory,
  // I could just pass the pointer at right address but for the time
  // being this will keep symmetry.
  MeshHandle loadMesh(const char *path) override;
  //TODO Note original function line suppressing the parameter, remove when implemented properly
  //const BoundingBox *getBoundingBoxes(uint32_t &outSize) const override {
  const BoundingBox *getBoundingBoxes(uint32_t &) const override {
    assert(0);
    return nullptr;
  };

  inline void renderMesh(const MeshHandle handle,
                         const VkCommandBuffer commandBuffer) const {
    const auto &runtime = getMeshRuntime(handle);
    if (runtime.indexBuffer != nullptr) {
      vkCmdBindIndexBuffer(commandBuffer, runtime.indexBuffer, 0,
                           VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(commandBuffer, runtime.indexCount, 1, 0, 0, 0);
    } else {
      vkCmdDraw(commandBuffer, runtime.indexCount, 1, 0, 0);
    }
  };

  void free(const MeshHandle handle) override;
  // vk methods
  VkMeshRuntime getMeshRuntime(const MeshHandle &handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const MeshData &data = m_meshPool.getConstRef(index);
    return data.meshRuntime;
  }
  void bindMesh(const MeshHandle handle, VkWriteDescriptorSet *set,
                const VkDescriptorSet descriptorSet,
                VkDescriptorBufferInfo *info, const uint32_t bindFlags,
                const uint32_t startIdx) const;

 private:
  inline void assertMagicNumber(const MeshHandle handle) const {
#ifdef SE_DEBUG
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_meshPool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for constant buffer");
#endif
  }
  MeshHandle getHandleFromName(const char *name) const override {
    MeshHandle handle{};
    m_nameToHandle.get(name, handle);
    assert(handle.isHandleValid());
    return handle;
  }

  inline const ResizableVector<BoundingBox> &getBoundingBoxes() const {
    return m_boundingBoxes;
  }

 private:
  SparseMemoryPool<MeshData> m_meshPool;
  HashMap<const char *, MeshHandle, hashString32> m_nameToHandle;

  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  ResizableVector<MeshUploadResource> m_uploadRequests;
  ResizableVector<BoundingBox> m_boundingBoxes;
};

}  // namespace SirEngine::vk
