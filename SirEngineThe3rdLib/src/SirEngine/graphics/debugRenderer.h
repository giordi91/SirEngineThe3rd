#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "SirEngine/materialManager.h"
#include "SirEngine/memory/cpu/hashMap.h"
#include "SirEngine/memory/gpu/gpuSlabAllocator.h"

enum class PRIMITIVE_TYPE { TRIANGLE, LINE, POINT };

struct BoundingBox;

namespace SirEngine {
// forward declare
struct Skeleton;
class AnimationPlayer;

class DebugRenderer {
 public:
  DebugRenderer();
  virtual ~DebugRenderer() = default;
  DebugRenderer(const DebugRenderer&) = delete;
  DebugRenderer& operator=(const DebugRenderer&) = delete;
  DebugRenderer(DebugRenderer&&) = delete;
  DebugRenderer& operator=(DebugRenderer&&) = delete;
  // override interface
  void initialize();
  void cleanup();

  void free();
  DebugDrawHandle drawPointsUniformColor(float* data, uint32_t sizeInByte,
                                         glm::vec4 color, float size,
                                         const char* debugName);
  DebugDrawHandle drawSkeleton(Skeleton* skeleton, glm::vec4 color,
                               float pointSize);

  void render(TextureHandle input, TextureHandle depth);
  DebugDrawHandle drawAnimatedBoundingBoxes(DebugDrawHandle handle,
                                            BoundingBox* data, int count,
                                            glm::vec4 color,
                                            const char* debugName);
  DebugDrawHandle drawAnimatedBoundingBoxFromFullPoints(
      const DebugDrawHandle handle, const glm::vec3* data, const int count,
      const glm::vec4 color, const char* debugName);
  DebugDrawHandle drawMatrix(const glm::mat4& mat, float size, glm::vec4 color,
                             const char* debugName);

  void drawBoundingBoxes(const BoundingBox* data, int count, glm::vec4 color);
  void drawLines(float* data, uint32_t sizeInByte, glm::vec4 color);
  void newFrame();

 private:
  void assureLinesTables(int slabCount);

 private:
  static constexpr uint32_t RESERVE_SIZE = 200;
  static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
  static constexpr uint32_t HANDLES_INITIAL_SIZE = 16;
  GPUSlabAllocator m_lineSlab[MAX_FRAMES_IN_FLIGHT];

  PSOHandle m_linePSO;
  RSHandle m_lineRS;
  ResizableVector<uint32_t> m_lineBindHandles;
};

}  // namespace SirEngine
