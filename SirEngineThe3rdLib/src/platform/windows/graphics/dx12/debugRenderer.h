#pragma once
#include "SirEngine/handle.h"
#include "SirEngine/materialManager.h"
#include <DirectXMath.h>
#include <unordered_map>
#include <vector>

namespace SirEngine::dx12 {
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

  void drawAABB(DirectX::XMFLOAT4 &minP, DirectX::XMFLOAT4 &maxP,
                DirectX::XMFLOAT4 &color, bool isPeristen);

  void drawRay(DirectX::XMFLOAT4 &rayOrigin, DirectX::XMFLOAT4 &rayDirection,
               DirectX::XMFLOAT4 &color, float rayLen, float originSize,
               bool isPeristen);

  DebugDrawHandle drawPointsUniformColor(float *data, uint32_t sizeInByte,
                                         DirectX::XMFLOAT4 color, float size,
                                         bool isPeristen,
                                         const char *debugName);
  DebugDrawHandle drawLinesUniformColor(float *data, uint32_t sizeInByte,
                                        DirectX::XMFLOAT4 color, float size,
                                        bool isPersistent, const char *debugName);

  void drawTriangle(float *point0, float *point1, float *point2,
                    DirectX::XMFLOAT4 color, bool isPeristen);

  void drawAABBs(float *float3aabs, int size, bool isPersisten);
  void drawPointsColor(std::vector<float> &data, float size, bool isPeristen);
  void drawGeoPointsColor(std::vector<float> &data, float size,
                          bool isPeristen);

  void render(const TextureHandle input);
  void clearUploadRequests();

private:
  DebugRenderer(const DebugRenderer &) = delete;
  DebugRenderer &operator=(const DebugRenderer &) = delete;

private:
  struct PSORSPair {
    PSOHandle pso;
    RSHandle rs;
  };

  std::unordered_map<uint32_t, std::vector<DebugPrimitive>>
      m_renderablesPersistant;
  std::unordered_map<uint32_t, std::vector<DebugPrimitive>> m_renderables;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  std::unordered_map<uint16_t, ShaderBind> m_shderTypeToShaderBind;
  std::vector<BufferUploadResource> m_uploadRequests;
  std::vector<DebugPrimitive> m_primToFree;
};

} // namespace SirEngine::dx12