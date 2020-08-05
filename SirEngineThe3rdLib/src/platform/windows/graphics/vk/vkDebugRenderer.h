#pragma once
#include "SirEngine/graphics/debugRenderer.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/cpu/hashMap.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"

namespace SirEngine::vk {

class VkDebugRenderer : public DebugRenderer {
  struct VkDebugPrimitive {
    ConstantBufferHandle m_cbHandle;
    BufferHandle m_bufferHandle;
    DescriptorHandle m_descriptorHandle;
    PRIMITIVE_TYPE m_primitiveType;
    uint32_t m_magicNumber;
    int m_primitiveToRender;
  };

 public:
  VkDebugRenderer();
  virtual ~VkDebugRenderer() = default;
  VkDebugRenderer(const VkDebugRenderer&) = delete;
  VkDebugRenderer& operator=(const VkDebugRenderer&) = delete;
  VkDebugRenderer(VkDebugRenderer&&) = delete;
  VkDebugRenderer& operator=(VkDebugRenderer&&) = delete;
  // override interface
  void initialize() override;
  void cleanup() override;

  void free(DebugDrawHandle handle) override;
  DebugDrawHandle drawPointsUniformColor(float* data, uint32_t sizeInByte,
                                         glm::vec4 color, float size,
                                         const char* debugName) override;
  DebugDrawHandle drawLinesUniformColor(float* data, uint32_t sizeInByte,
                                        glm::vec4 color, float size,
                                        const char* debugName) override;
  DebugDrawHandle drawSkeleton(Skeleton* skeleton, glm::vec4 color,
                               float pointSize) override;
  DebugDrawHandle drawAnimatedSkeleton(DebugDrawHandle handle,
                                       AnimationPlayer* state, glm::vec4 color,
                                       float pointSize) override;
  void render(TextureHandle input, TextureHandle depth) override;
  DebugDrawHandle drawBoundingBoxes(const BoundingBox* data, int count,
                                    glm::vec4 color,
                                    const char* debugName) override;
  DebugDrawHandle drawAnimatedBoundingBoxes(DebugDrawHandle handle,
                                            BoundingBox* data, int count,
                                            glm::vec4 color,
                                            const char* debugName) override;
  DebugDrawHandle drawAnimatedBoundingBoxFromFullPoints(
      const DebugDrawHandle handle, const glm::vec3* data, const int count,
      const glm::vec4 color, const char* debugName) override;
  DebugDrawHandle drawMatrix(const glm::mat4& mat, float size, glm::vec4 color,
                  const char* debugName) override;

  void updateBoundingBoxesData(DebugDrawHandle handle, const BoundingBox* data, int count) override;
 private:
  inline void assertMagicNumber(const DebugDrawHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);

    DebugTracker tracker;
    const bool found = m_trackers.get(handle.handle, tracker);
    assert(found);
    assert(tracker.magicNumber == magic &&
           "invalid magic handle for debug tracker buffer");
  }
  inline void assertPoolMagicNumber(const DebugDrawHandle handle) {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(m_primitivesPool[idx].m_magicNumber == magic &&
           "invalid magic handle for material data");
  }

 private:
  static constexpr uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  SparseMemoryPool<VkDebugPrimitive> m_primitivesPool;
  HashMap<uint32_t, DebugTracker, hashUint32> m_trackers;
};

}  // namespace SirEngine::vk
