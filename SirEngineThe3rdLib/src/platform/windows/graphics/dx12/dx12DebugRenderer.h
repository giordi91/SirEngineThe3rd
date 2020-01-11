#pragma once
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

#include "SirEngine/graphics/debugRenderer.h"
#include "SirEngine/handle.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/dx12MaterialManager.h"

namespace SirEngine {

// forward declare
struct Skeleton;
class AnimationPlayer;

namespace dx12 {

enum PRIMITIVE_TYPE { TRIANGLE, LINE, POINT };
struct DebugPrimitive {
  // slow I know but for the time being will get the job done
  ConstantBufferHandle cbHandle;
  ID3D12Resource *buffer;
  int primitiveToRender;
  PRIMITIVE_TYPE primitiveType;
  DescriptorPair srv;
  uint64_t fence;
};

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

class Dx12DebugRenderer : public DebugRenderer {
  struct BufferUploadResource final {
    ID3D12Resource *uploadBuffer = nullptr;
    UINT64 fence = 0;
  };

 public:
  Dx12DebugRenderer():DebugRenderer(){};
  Dx12DebugRenderer(const Dx12DebugRenderer &) = delete;
  Dx12DebugRenderer &operator=(const Dx12DebugRenderer &) = delete;
  Dx12DebugRenderer(Dx12DebugRenderer &&) = delete;
  Dx12DebugRenderer &operator=(Dx12DebugRenderer &&) = delete;
  void initialize() override;

  void cleanup() override {
    // for (auto &deb : m_persistante_q) {
    //  deb.buff->Release();
    //}
    // m_persistante_q.clear();
  }

  DebugDrawHandle drawPointsUniformColor(float *data, uint32_t sizeInByte,
                                         glm::vec4 color, float size,
                                         const char *debugName) override;
  DebugDrawHandle drawLinesUniformColor(float *data, uint32_t sizeInByte,
                                        glm::vec4 color, float size,
                                        const char *debugName) override;
  DebugDrawHandle drawSkeleton(Skeleton *skeleton, glm::vec4 color,
                               float pointSize);
  DebugDrawHandle drawAnimatedSkeleton(DebugDrawHandle handle,
                                       AnimationPlayer *state, glm::vec4 color,
                                       float pointSize) override;

  void render(TextureHandle input, TextureHandle depth) override;
  void clearUploadRequests() override;
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

  void drawMatrix(const glm::mat4 &mat, float size, glm::vec4 color,
                  const char *debugName) override;

 private:
  void renderQueue(
      std::unordered_map<uint32_t, std::vector<DebugPrimitive>> &inQueue,
      const TextureHandle input, const TextureHandle depth);

  static bool isCompound(const DebugDrawHandle handle) {
    return (handle.handle & (1 << 31)) > 0;
  }
  inline uint32_t getIndexFromHandle(const DebugDrawHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(const DebugDrawHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }

  inline void assertMagicNumber(const DebugDrawHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const auto found = m_trackers.find(handle.handle);
    assert(found != m_trackers.end());
    assert(found->second.magicNumber == magic &&
           "invalid magic number for debug tracker");
  }

 private:
  struct PSORSPair {
    PSOHandle pso;
    RSHandle rs;
  };

  std::unordered_map<uint32_t, std::vector<DebugPrimitive>> m_renderables;
  std::unordered_map<uint32_t, DebugTracker> m_trackers;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  std::unordered_map<uint16_t, ShaderBind> m_shderTypeToShaderBind;
  std::vector<BufferUploadResource> m_uploadRequests;
  std::vector<DebugPrimitive> m_primToFree;
};

}  // namespace dx12
}  // namespace SirEngine