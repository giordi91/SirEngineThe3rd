#pragma once

#include <glm/vec3.hpp>
#include <vector>

#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"

namespace SirEngine::graphics {


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
  ConstantBufferHandle m_grassConfigHandle{};
  BindingTableHandle m_bindingTable{};


  MaterialHandle m_grassMaterial{};
  TextureHandle m_windTexture{};
  DebugDrawHandle m_debugHandle{};
  PSOHandle m_pso;
  RSHandle m_rs;

  std::vector<char> m_binaryData;
  std::vector<glm::vec3> m_tilesPoints;
  std::vector<int> m_tilesIndices;

  static const uint32_t MAX_GRASS_PER_SIDE = 60;
};
}  // namespace SirEngine::graphics
