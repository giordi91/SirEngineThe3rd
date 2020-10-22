#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "SirEngine/memory/gpu/gpuSlabAllocator.h"

enum class PRIMITIVE_TYPE { TRIANGLE, LINE, POINT };

struct BoundingBox;

namespace SirEngine {
class CameraController;
struct Skeleton;
class AnimationPlayer;

class DebugRenderer final {
 public:
  DebugRenderer();
  virtual ~DebugRenderer() = default;
  DebugRenderer(const DebugRenderer&) = delete;
  DebugRenderer& operator=(const DebugRenderer&) = delete;
  DebugRenderer(DebugRenderer&&) = delete;
  DebugRenderer& operator=(DebugRenderer&&) = delete;

  void initialize();
  void cleanup();
  void free();

  void render(uint32_t renderWidth,uint32_t renderHeight);
  void drawBoundingBoxes(const BoundingBox* data, int count, glm::vec4 color);
  void drawLines(const float* data, uint32_t sizeInByte, glm::vec4 color);
  void newFrame();
  void drawCamera(const CameraController* camera, glm::vec4 color);

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
