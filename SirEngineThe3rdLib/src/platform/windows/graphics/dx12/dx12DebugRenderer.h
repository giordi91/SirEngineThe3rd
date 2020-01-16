#pragma once
#include <glm/glm.hpp>

#include "SirEngine/graphics/debugRenderer.h"
#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/memory/hashMap.h"
#include "SirEngine/memory/sparseMemoryPool.h"

namespace SirEngine {

// forward declare
struct Skeleton;
class AnimationPlayer;

namespace dx12 {

struct Dx12DebugPrimitive {
  // slow I know but for the time being will get the job done
  ConstantBufferHandle m_cbHandle;
  BufferHandle m_bufferHandle;
  int m_primitiveToRender;
  PRIMITIVE_TYPE m_primitiveType;
  uint32_t m_magicNumber;
  uint32_t m_sizeInByte;
};

struct DebugTracker {
  uint32_t m_magicNumber;
  uint32_t m_compoundCount;
  DebugDrawHandle *m_compoundHandles;
};

class Dx12DebugRenderer : public DebugRenderer {

public:
  // TODO cleanup creation, this is probably going away
  Dx12DebugRenderer()
      : DebugRenderer(), m_trackers(200), m_primitivesPool(200){};
  virtual ~Dx12DebugRenderer() = default;
  Dx12DebugRenderer(const Dx12DebugRenderer &) = delete;
  Dx12DebugRenderer &operator=(const Dx12DebugRenderer &) = delete;
  Dx12DebugRenderer(Dx12DebugRenderer &&) = delete;
  Dx12DebugRenderer &operator=(Dx12DebugRenderer &&) = delete;
  void initialize() override;
  void free(DebugDrawHandle handle) override;

  void cleanup() override {
    // TODO properly implement cleanup
  }

  DebugDrawHandle drawPointsUniformColor(float *data, uint32_t sizeInByte,
                                         glm::vec4 color, float size,
                                         const char *debugName) override;
  DebugDrawHandle drawLinesUniformColor(float *data, uint32_t sizeInByte,
                                        glm::vec4 color, float size,
                                        const char *debugName) override;
  DebugDrawHandle drawSkeleton(Skeleton *skeleton, glm::vec4 color,
                               float pointSize) override;
  DebugDrawHandle drawAnimatedSkeleton(DebugDrawHandle handle,
                                       AnimationPlayer *state, glm::vec4 color,
                                       float pointSize) override;

  void render(TextureHandle input, TextureHandle depth) override;
  DebugDrawHandle drawBoundingBoxes(BoundingBox *data, int count,
                                    glm::vec4 color,
                                    const char *debugName) override;

  DebugDrawHandle drawAnimatedBoundingBoxes(DebugDrawHandle handle,
                                            BoundingBox *data, int count,
                                            glm::vec4 color,
                                            const char *debugName) override;

  DebugDrawHandle drawAnimatedBoundingBoxFromFullPoints(
      const DebugDrawHandle handle, const glm::vec3 *data, const int count,
      const glm::vec4 color, const char *debugName) override;

  DebugDrawHandle drawMatrix(const glm::mat4 &mat, float size, glm::vec4 color,
                             const char *debugName) override;

private:
  static bool isCompound(const DebugDrawHandle handle) {
    return (handle.handle & (1 << 31)) > 0;
  }

  inline void assertMagicNumber(const DebugDrawHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);

    DebugTracker tracker;
    bool found = m_trackers.get(handle.handle, tracker);
    assert(found);
    assert(tracker.m_magicNumber == magic &&
           "invalid magic handle for debug tracker buffer");
  }
  inline void assertPoolMagicNumber(const DebugDrawHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    const auto &data = m_primitivesPool.getConstRef(idx);
    assert(data.m_magicNumber == magic &&
           "invalid magic handle for material data");
  }

private:
  HashMap<uint32_t, DebugTracker, hashUint32> m_trackers;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  SparseMemoryPool<Dx12DebugPrimitive> m_primitivesPool;
};

} // namespace dx12
} // namespace SirEngine