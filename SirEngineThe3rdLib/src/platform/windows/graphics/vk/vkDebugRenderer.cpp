#include "platform/windows/graphics/vk/vkDebugRenderer.h"

namespace SirEngine::vk {
void VkDebugRenderer::initialize() {}

void VkDebugRenderer::cleanup() {}

DebugDrawHandle VkDebugRenderer::drawPointsUniformColor(float* data,
                                                        uint32_t sizeInByte,
                                                        glm::vec4 color,
                                                        float size,
                                                        const char* debugName) {
  assert(0);
  return {};
}

DebugDrawHandle VkDebugRenderer::drawLinesUniformColor(float* data,
                                                       uint32_t sizeInByte,
                                                       glm::vec4 color,
                                                       float size,
                                                       const char* debugName) {
  assert(0);
  return {};
}

DebugDrawHandle VkDebugRenderer::drawSkeleton(Skeleton* skeleton,
                                              glm::vec4 color,
                                              float pointSize) {
  assert(0);
  return {};
}

DebugDrawHandle VkDebugRenderer::drawAnimatedSkeleton(DebugDrawHandle handle,
                                                      AnimationPlayer* state,
                                                      glm::vec4 color,
                                                      float pointSize) {
  assert(0);
  return {};
}

void VkDebugRenderer::render(TextureHandle input, TextureHandle depth) {
  assert(0);
}

void VkDebugRenderer::clearUploadRequests() {}

DebugDrawHandle VkDebugRenderer::drawBoundingBoxes(BoundingBox* data, int count,
                                                   glm::vec4 color,
                                                   const char* debugName) {
  assert(0);
  return {};
}  // namespace SirEngine::vk

DebugDrawHandle VkDebugRenderer::drawAnimatedBoundingBoxes(
    DebugDrawHandle handle, BoundingBox* data, int count, glm::vec4 color,
    const char* debugName) {
  assert(0);
  return {};
}

DebugDrawHandle VkDebugRenderer::drawAnimatedBoundingBoxFromFullPoints(
    const DebugDrawHandle handle, const glm::vec3* data, const int count,
    const glm::vec4 color, const char* debugName) {
  assert(0);
  return {};
}

void VkDebugRenderer::drawMatrix(const glm::mat4& mat, float size,
                                 glm::vec4 color, const char* debugName) {
  assert(0);
}
}  // namespace SirEngine::vk
