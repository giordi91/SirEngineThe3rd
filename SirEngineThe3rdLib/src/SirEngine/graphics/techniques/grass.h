#pragma once

#include <glm/vec3.hpp>
#include <vector>

#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include "SirEngine/log.h"

namespace SirEngine::graphics {

class GrassTechnique final : public GNodeCallback {
 public:
  GrassTechnique() = default;
  virtual ~GrassTechnique() = default;
  void setup(uint32_t id) override;
  void render(uint32_t id ,BindingTableHandle passHandle) override;
  void clear(uint32_t id) override;

 public:
	 static constexpr uint32_t GRASS_TECHNIQUE_FORWARD = 1;
	 static constexpr uint32_t GRASS_TECHNIQUE_SHADOW = 2;

 private:
  void tileDebug();

 private:
  GrassConfig m_grassConfig{};
  GrassConfig m_grassConfigOld{};

  BufferHandle m_grassBuffer{};
  BufferHandle m_tilesPointsHandle{};
  BufferHandle m_tilesIndicesHandle{};
  ConstantBufferHandle m_grassConfigHandle{};
  BindingTableHandle m_bindingTable{};

  MaterialHandle m_grassMaterial{};
  TextureHandle m_windTexture{};
  TextureHandle m_albedoTexture{};
  DebugDrawHandle m_debugHandle{};
  PSOHandle m_pso;
  RSHandle m_rs;

  std::vector<char> m_binaryData;
  std::vector<glm::vec3> m_tilesPoints;
  std::vector<int> m_tilesIndices;

  static const uint32_t MAX_GRASS_PER_SIDE = 60;
};
}  // namespace SirEngine::graphics
