#pragma once

#include <glm/vec3.hpp>

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine::graphics {

struct GrassConfig {
  glm::vec3 gridOrigin;
  int tilesPerSide;
  float tileSize;
};

class GrassTechnique : public GNodeCallback {
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
  MaterialHandle m_grassMaterial{};
  static const uint32_t maxGrassPerSide = 60;
  TextureHandle m_windTexture{};
  DebugDrawHandle m_debugHandle{};
};
}  // namespace SirEngine::graphics
