#include "platform/windows/graphics/vk/vkDebugRenderer.h"

#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "platform/windows/graphics/vk/vkRootSignatureManager.h"

namespace SirEngine::vk {
void VkDebugRenderer::initialize() {
  // lets build the map of rs and pso
  // points single color
  PSOHandle psoHandle =
      vk::PSO_MANAGER->getHandleFromName("debugDrawPointsSingleColorPSO");
  RSHandle rsHandle = vk::PIPELINE_LAYOUT_MANAGER->getHandleFromName(
      "debugDrawPointsSingleColorRS");

  m_shderTypeToShaderBind.insert(
      static_cast<uint16_t>(SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR),
      ShaderBind{rsHandle, psoHandle});

  psoHandle =
      vk::PSO_MANAGER->getHandleFromName("debugDrawLinesSingleColorPSO");
  rsHandle = vk::PIPELINE_LAYOUT_MANAGER->getHandleFromName(
      "debugDrawLinesSingleColorRS");

  m_shderTypeToShaderBind.insert(
      static_cast<uint16_t>(SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR),
      ShaderBind{rsHandle, psoHandle});
}

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
