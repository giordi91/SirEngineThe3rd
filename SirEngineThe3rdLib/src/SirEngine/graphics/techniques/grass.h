#pragma once

#include <glm/vec3.hpp>
#include <vector>

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine::graphics {

struct GrassConfig {
  glm::vec3 gridOrigin;
  int tilesPerSide;
  float tileSize;
  int sourceDataTileCount;
  int seed;
};

class GrassTechnique final : public GNodeCallback {
 public:
  void setup() override;
  void render(BindingTableHandle passHandle) override;
  void clear() override;

 private:
  void tileDebug();

 private:
  GrassConfig m_grassConfig{};
  GrassConfig m_grassConfigOld{};
  BufferHandle m_grassBuffer{};
  BufferHandle m_tilesPointsHandle{};
  BufferHandle m_tilesIndicesHandle{};
  MaterialHandle m_grassMaterial{};
  TextureHandle m_windTexture{};
  DebugDrawHandle m_debugHandle{};
  std::vector<char> m_binaryData;
  std::vector<glm::vec3> m_tilesPoints;
  std::vector<int> m_tilesIndices;

  static const uint32_t MAX_GRASS_PER_SIDE = 60;
};
}  // namespace SirEngine::graphics
