#pragma once

#include <glm/vec3.hpp>
#include <vector>

#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/handle.h"

namespace SirEngine::graphics {

class GrassTechnique final : public GNodeCallback {
 public:
  GrassTechnique() = default;
  virtual ~GrassTechnique() = default;
  void setup(uint32_t id) override;
  void passRender(uint32_t id, BindingTableHandle passHandle) override;
  void clear(uint32_t id) override;
  void prePassRender(uint32_t id) override;

 public:
  static constexpr uint32_t GRASS_TECHNIQUE_FORWARD = 1;
  static constexpr uint32_t GRASS_TECHNIQUE_SHADOW = 2;

 private:
  void buildBindingTables();
  void renderGroundPlane(BindingTableHandle passHandle) const;
  void performCulling();
  void tileDebug();

 private:
  GrassConfig m_grassConfig{};
  GrassConfig m_grassConfigOld{};

  BufferHandle m_grassBuffer{};
  BufferHandle m_tilesPointsHandle{};
  BufferHandle m_tilesIndicesHandle{};
  ConstantBufferHandle m_grassConfigHandle{};
  BindingTableHandle m_bindingTable[4]={};
  BindingTableHandle m_groundBindingTable{};

  TextureHandle m_windTexture{};
  TextureHandle m_albedoTexture{};
  TextureHandle m_groundAlbedoTexture{};

  PSOHandle m_pso;
  RSHandle m_rs;

  PSOHandle m_groundPso;
  RSHandle m_groundRs;

  std::vector<char> m_binaryData;
  std::vector<glm::vec3> m_tilesPoints;
  std::vector<int> m_tilesIndices;
  std::vector<glm::vec4> m_tilePositions;
  BindingTableHandle m_scanBindingTable{};
  BindingTableHandle m_clearBindingTable{};
  RSHandle m_grassCullScanRs{};
  PSOHandle m_grassCullScanPso{};
  RSHandle m_grassClearRs{};
  PSOHandle m_grassClearPso{};
  BufferHandle m_cullingInBuffer{};
  BufferHandle m_outTiles{};
  BufferHandle m_lodBuffer[4] = {};

  static const uint32_t MAX_GRASS_PER_SIDE = 60;
  std::vector<BoundingBox> m_tiles;
};
}  // namespace SirEngine::graphics
