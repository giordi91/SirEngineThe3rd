#pragma once
#include "SirEngine/graphics/debugRenderer.h"
#include "SirEngine/memory/hashMap.h"
#include "platform/windows/graphics/vk/volk.h"

#include <vector>
#include <unordered_map>
#include "SirEngine/materialManager.h"

namespace SirEngine::vk {

class VkDebugRenderer : public DebugRenderer {
  struct VkDebugPrimitive {
    // slow I know but for the time being will get the job done
    ConstantBufferHandle cbHandle;
    BufferHandle bufferHandle;
    DescriptorHandle descriptorHandle;
    VkPipelineLayout layout;
    int primitiveToRender;
    PRIMITIVE_TYPE primitiveType;
    // DescriptorPair srv;
    uint64_t fence;
  };

 public:
  VkDebugRenderer()
      : DebugRenderer(),
        m_shderTypeToShaderBind(RESERVE_SIZE),
        m_trackers(RESERVE_SIZE){};
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
  void VkDebugRenderer::renderQueue(
      std::unordered_map<uint32_t, std::vector<VkDebugPrimitive>>& inQueue,
      const TextureHandle input, const TextureHandle depth);

 private:
  static constexpr uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  HashMap<uint16_t, ShaderBind, hashUint16> m_shderTypeToShaderBind;
  std::unordered_map<uint32_t, std::vector<VkDebugPrimitive>> m_renderables;
  HashMap<uint32_t, DebugTracker, hashUint32> m_trackers;
};

}  // namespace SirEngine::vk
