#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "SirEngine/handle.h"

struct BoundingBox;

namespace SirEngine {

// forward declare
struct Skeleton;
class AnimationPlayer;

enum class PRIMITIVE_TYPE { TRIANGLE, LINE, POINT };

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

class DebugRenderer {
 public:
  DebugRenderer() = default;
  virtual ~DebugRenderer() = default;
  virtual void newFrame() = 0;
  DebugRenderer(const DebugRenderer &) = delete;
  DebugRenderer &operator=(const DebugRenderer &) = delete;
  DebugRenderer(DebugRenderer &&) = delete;
  DebugRenderer &operator=(DebugRenderer &&) = delete;

  virtual void initialize() = 0;
  virtual void cleanup() = 0;
  virtual void free(DebugDrawHandle handle) = 0;

  virtual DebugDrawHandle drawPointsUniformColor(float *data,
                                                 uint32_t sizeInByte,
                                                 glm::vec4 color, float size,
                                                 const char *debugName) = 0;
  virtual DebugDrawHandle drawLinesUniformColor(float *data,
                                                uint32_t sizeInByte,
                                                glm::vec4 color, float size,
                                                const char *debugName) = 0;
  virtual void drawLines(float *data, uint32_t sizeInByte, glm::vec4 color,
                         float size, const char *debugName) = 0;

  virtual DebugDrawHandle drawSkeleton(Skeleton *skeleton, glm::vec4 color,
                                       float pointSize) = 0;
  virtual DebugDrawHandle drawAnimatedSkeleton(DebugDrawHandle handle,
                                               AnimationPlayer *state,
                                               glm::vec4 color,
                                               float pointSize) = 0;

  virtual void render(TextureHandle input, TextureHandle depth) = 0;
  virtual DebugDrawHandle drawBoundingBoxes(const BoundingBox *data, int count,
                                            glm::vec4 color,
                                            const char *debugName) = 0;
  virtual void updateBoundingBoxesData(DebugDrawHandle handle,
                                       const BoundingBox *data, int count) = 0;

  virtual DebugDrawHandle drawAnimatedBoundingBoxes(DebugDrawHandle handle,
                                                    BoundingBox *data,
                                                    int count, glm::vec4 color,
                                                    const char *debugName) = 0;

  virtual DebugDrawHandle drawAnimatedBoundingBoxFromFullPoints(
      const DebugDrawHandle handle, const glm::vec3 *data, const int count,
      const glm::vec4 color, const char *debugName) = 0;

  virtual DebugDrawHandle drawMatrix(const glm::mat4 &mat, float size,
                                     glm::vec4 color,
                                     const char *debugName) = 0;
};

}  // namespace SirEngine
