#pragma once
#include "platform/windows/graphics/dx12/dx12DebugRenderer.h"

namespace SirEngine::vk {

class VkDebugRenderer : public DebugRenderer {
 public:
  VkDebugRenderer(): DebugRenderer(),m_shderTypeToShaderBind(RESERVE_SIZE){};
  virtual ~VkDebugRenderer() = default;
  VkDebugRenderer(const VkDebugRenderer&) = delete;
  VkDebugRenderer& operator=(const VkDebugRenderer&) = delete;
  VkDebugRenderer(VkDebugRenderer&&) = delete;
  VkDebugRenderer& operator=(VkDebugRenderer&&) = delete;
  // override interface
  void initialize() override;
  void cleanup() override;
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
  void clearUploadRequests() override;
  DebugDrawHandle drawBoundingBoxes(BoundingBox* data, int count,
                                    glm::vec4 color,
                                    const char* debugName) override;
  DebugDrawHandle drawAnimatedBoundingBoxes(DebugDrawHandle handle,
                                            BoundingBox* data, int count,
                                            glm::vec4 color,
                                            const char* debugName) override;
  DebugDrawHandle drawAnimatedBoundingBoxFromFullPoints(
      const DebugDrawHandle handle, const glm::vec3* data, const int count,
      const glm::vec4 color, const char* debugName) override;
  void drawMatrix(const glm::mat4& mat, float size, glm::vec4 color,
                  const char* debugName) override;
private:
  static constexpr uint32_t RESERVE_SIZE =20;
  HashMap<uint16_t, ShaderBind, hashUint16> m_shderTypeToShaderBind;
};

}  // namespace SirEngine::vk
