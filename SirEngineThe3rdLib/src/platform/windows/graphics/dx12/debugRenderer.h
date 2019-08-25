#pragma once
#include "SirEngine/handle.h"
#include "SirEngine/materialManager.h"
#include <DirectXMath.h>
#include <unordered_map>
#include <vector>

namespace SirEngine {

// forward declare
struct Skeleton;
struct AnimState;
namespace dx12 {

enum PRIMITIVE_TYPE { TRIANGLE, LINE, POINT };
struct DebugPrimitive {
  // slow I know but for the time being will get the job done
  ConstantBufferHandle cbHandle;
  ID3D12Resource *buffer;
  D3D12_VERTEX_BUFFER_VIEW bufferView;
  int primitiveToRender;
  PRIMITIVE_TYPE primitiveType;
  DescriptorPair srv;
  uint64_t fence;
};

struct DebugCompoundTracker {
  uint32_t count : 16;
  uint32_t startIdx : 16;
};

class DebugRenderer {
  struct BufferUploadResource final {
    ID3D12Resource *uploadBuffer = nullptr;
    UINT64 fence = 0;
  };

public:
  DebugRenderer() = default;
  void init();
  void cleanPerFrame();

  void cleanup() {
    // for (auto &deb : m_persistante_q) {
    //  deb.buff->Release();
    //}
    // m_persistante_q.clear();
  }

  DebugDrawHandle drawPointsUniformColor(float *data, uint32_t sizeInByte,
                                         DirectX::XMFLOAT4 color, float size,
                                         const char *debugName);
  DebugDrawHandle drawLinesUniformColor(float *data, uint32_t sizeInByte,
                                        DirectX::XMFLOAT4 color, float size,
                                        const char *debugName);
  DebugDrawHandle drawSkeleton(Skeleton *skeleton, DirectX::XMFLOAT4 color,
                               float pointSize);
  DebugDrawHandle drawAnimatedSkeleton(DebugDrawHandle handle, AnimState *state,
                                       DirectX::XMFLOAT4 color,
                                       float pointSize);

  void render(const TextureHandle input, const TextureHandle depth);
  void clearUploadRequests();

private:
  DebugRenderer(const DebugRenderer &) = delete;
  DebugRenderer &operator=(const DebugRenderer &) = delete;
  void renderQueue(
      std::unordered_map<uint32_t, std::vector<DebugPrimitive>> &inQueue,
      const TextureHandle input, const TextureHandle depth);

  inline bool isCompound(const DebugDrawHandle handle) {
    return (handle.handle & (1 << 31)) > 0;
  }

private:
  struct PSORSPair {
    PSOHandle pso;
    RSHandle rs;
  };

  std::unordered_map<uint32_t, std::vector<DebugPrimitive>> m_renderables;
  std::unordered_map<uint32_t, DebugCompoundTracker> m_trackers;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  std::unordered_map<uint16_t, ShaderBind> m_shderTypeToShaderBind;
  std::vector<BufferUploadResource> m_uploadRequests;
  std::vector<DebugPrimitive> m_primToFree;
};

} // namespace dx12
} // namespace SirEngine