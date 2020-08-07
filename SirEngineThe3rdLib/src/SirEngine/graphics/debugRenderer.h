#pragma once
#include "SirEngine/memory/cpu/hashMap.h"


#include "SirEngine/memory/gpu/gpuSlabAllocator.h"
#include "SirEngine/materialManager.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

enum class PRIMITIVE_TYPE { TRIANGLE, LINE, POINT };

struct BoundingBox;

namespace SirEngine {
// forward declare
struct Skeleton;
class AnimationPlayer;


struct DebugTracker {
  uint32_t queue;
  uint32_t index : 16;
  uint32_t magicNumber : 16;
  // this part refer to a compound handle only
  uint32_t compoundCount;
  DebugDrawHandle *compoundHandles;
  void *mappedData;
  uint32_t sizeInBtye;
};

class DebugRenderer  {
  struct VkDebugPrimitive {
    ConstantBufferHandle m_cbHandle;
    BufferHandle m_bufferHandle;
    DescriptorHandle m_descriptorHandle;
    PRIMITIVE_TYPE m_primitiveType;
    uint32_t m_magicNumber;
    int m_primitiveToRender;
  };

 public:
  DebugRenderer();
  virtual ~DebugRenderer() = default;
  DebugRenderer(const DebugRenderer&) = delete;
  DebugRenderer& operator=(const DebugRenderer&) = delete;
  DebugRenderer(DebugRenderer&&) = delete;
  DebugRenderer& operator=(DebugRenderer&&) = delete;
  // override interface
  void initialize() ;
  void cleanup() ;

  void free(DebugDrawHandle handle) ;
  DebugDrawHandle drawPointsUniformColor(float* data, uint32_t sizeInByte,
                                         glm::vec4 color, float size,
                                         const char* debugName) ;
  DebugDrawHandle drawSkeleton(Skeleton* skeleton, glm::vec4 color,
                               float pointSize) ;

  void render(TextureHandle input, TextureHandle depth) ;
  DebugDrawHandle drawBoundingBoxes(const BoundingBox* data, int count,
                                    glm::vec4 color,
                                    const char* debugName) ;
  DebugDrawHandle drawAnimatedBoundingBoxes(DebugDrawHandle handle,
                                            BoundingBox* data, int count,
                                            glm::vec4 color,
                                            const char* debugName) ;
  DebugDrawHandle drawAnimatedBoundingBoxFromFullPoints(
      const DebugDrawHandle handle, const glm::vec3* data, const int count,
      const glm::vec4 color, const char* debugName) ;
  DebugDrawHandle drawMatrix(const glm::mat4& mat, float size, glm::vec4 color,
                  const char* debugName) ;

  void drawLines(float* data, uint32_t sizeInByte, glm::vec4 color, float size,
	  const char* debugName) ;
  void newFrame() ;

  void updateBoundingBoxesData(DebugDrawHandle handle, const BoundingBox* data, int count) ;
 private:

  void assureLinesTables(int slabCount);

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
  static constexpr uint32_t MAX_FRAMES_IN_FLIGHT= 3;
  static constexpr uint32_t HANDLES_INITIAL_SIZE= 16;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  SparseMemoryPool<VkDebugPrimitive> m_primitivesPool;
  HashMap<uint32_t, DebugTracker, hashUint32> m_trackers;
  GPUSlabAllocator m_lineSlab[MAX_FRAMES_IN_FLIGHT];
  uint32_t m_linesPrimitives = 0;
  PSOHandle m_linePSO;
  RSHandle m_lineRS;
  ResizableVector<uint32_t> m_lineBindHandles;
};

}  // namespace SirEngine::vk
