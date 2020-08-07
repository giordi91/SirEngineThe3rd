#include "SirEngine/graphics/techniques/grass.h"

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/bufferManager.h"
#include "SirEngine/constantBufferManager.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/graphics/debugRenderer.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/interopData.h"
#include "SirEngine/log.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"
#include "SirEngine/textureManager.h"

namespace SirEngine::graphics {

static const char *GRASS_RS = "grassForwardRS";
static const char *GRASS_PSO = "grassForwardPSO";
static const char *GRASS_PLANE_RS = "grassPlaneRS";
static const char *GRASS_PLANE_PSO = "grassPlanePSO";

void GrassTechnique::setup(const uint32_t id) {
  if (id != GRASS_TECHNIQUE_FORWARD) {
    SE_CORE_WARN("Anything other than forward unsupported for grass for now");
    return;
  }

  m_grassConfig.tilesPerSide = 25;
  m_grassConfig.tileSize = 5;
  m_grassConfig.grassBend = 0.41f;
  m_grassConfig.width = 0.1f;
  m_grassConfig.height = 1.0f;
  m_grassConfig.widthRandom = 0.02f;
  m_grassConfig.heightRandom = 0.425f;
  m_grassConfig.windStrength = 0.415f;
  m_grassConfig.windFrequency = {0.05, 0.05};
  m_grassConfig.bladeForward = 1.2f;
  m_grassConfig.bladeCurvatureAmount = 1.3f;
  m_grassConfig.pointsPerTile = 500;
  m_grassConfig.pointsPerSourceTile = 10000;
  m_grassConfig.gridOrigin = {0, 0, 0};
  m_grassConfig.metalness = 0.0;
  m_grassConfig.roughness = 0.338;
  m_grassConfig.baseColor = {0.02, 0.25, 0.001};
  m_grassConfig.tipColor = {0.02, 0.13, 0.019};

  m_rs = globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(GRASS_RS);
  m_pso = globals::PSO_MANAGER->getHandleFromName(GRASS_PSO);

  m_groundRs =
      globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(GRASS_PLANE_RS);
  m_groundPso = globals::PSO_MANAGER->getHandleFromName(GRASS_PLANE_PSO);

  // lets read the grass file
  const char *grassFile = "../data/processed/grass/grass.points";

  readAllBytes(grassFile, m_binaryData);

  const auto *const mapper =
      getMapperData<PointTilerMapperData>(m_binaryData.data());
  auto *pointData = reinterpret_cast<float *>(
      m_binaryData.data() + sizeof(BinaryFileHeader) + mapper->nameSizeInByte);
  m_grassConfig.sourceDataTileCount = mapper->tileCount;
  // used to find changes in config
  m_grassConfigOld = m_grassConfig;
  globals::INTEROP_DATA->registerData("grassConfig", &m_grassConfig);

  /*
  std::vector<BoundingBox> tiles;
  tiles.reserve(MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE);
  memset(tiles.data(), 0,
         MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE * sizeof(BoundingBox));

  float halfSize = (static_cast<float>(m_grassConfig.tilesPerSide) / 2.0f) *
                   m_grassConfig.tileSize;
  glm::vec3 minCorner =
      m_grassConfig.gridOrigin - glm::vec3{halfSize, 0.0f, halfSize};

  float tw = m_grassConfig.tileSize;
  BoundingBox box{};
  for (int tileY = 0; tileY < m_grassConfig.tilesPerSide; ++tileY) {
    for (int tileX = 0; tileX < m_grassConfig.tilesPerSide; ++tileX) {
      box.min = minCorner + glm::vec3{tw * tileX, 0, tw * tileY};
      box.max = minCorner + glm::vec3{tw * (tileX + 1), 0.1f, tw * (tileY + 1)};
      tiles.emplace_back(box);
      m_tilesPoints.push_back(box.min);
    }
  }

  int runtimeTilesCount = MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE;
  m_tilesIndices.reserve(runtimeTilesCount);
  for (int tileY = 0; tileY < m_grassConfig.tilesPerSide; ++tileY) {
    for (int tileX = 0; tileX < m_grassConfig.tilesPerSide; ++tileX) {
      int value =
          (rand() * static_cast<int>(m_grassConfig.sourceDataTileCount) /
           RAND_MAX);
      m_tilesIndices.push_back(value);
    }
  }

  m_windTexture = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/grass/wind.texture");

  m_albedoTexture = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/grass/grassAlbedo.texture");

  m_groundAlbedoTexture = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/grass/grassGround.texture");

  // m_debugHandle = globals::DEBUG_RENDERER->drawBoundingBoxes(
  //    tiles.data(), MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE,
  //    glm::vec4(1, 0, 0, 1), "debugGrassTiles");

  m_tilesPointsHandle = globals::BUFFER_MANAGER->allocate(
      mapper->pointsSizeInByte, pointData, "grassBuffer",
      mapper->tileCount * mapper->pointsPerTile, sizeof(float) * 3,
      BufferManager::STORAGE_BUFFER);
  m_tilesIndicesHandle = globals::BUFFER_MANAGER->allocate(
      sizeof(uint32_t) * runtimeTilesCount, m_tilesIndices.data(),
      "grassTilesIndicesBuffer", runtimeTilesCount, sizeof(uint32_t),
      BufferManager::STORAGE_BUFFER);
  m_grassConfigHandle = globals::CONSTANT_BUFFER_MANAGER->allocate(
      sizeof(m_grassConfig),
      ConstantBufferManager::CONSTANT_BUFFER_FLAG_BITS::UPDATED_EVERY_FRAME,
      &m_grassConfig);

  // build binding table
  graphics::BindingDescription descriptions[] = {
      {0, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,  // tiles points
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX},
      {1, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,  // tiles ids
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX},
      {2, GRAPHIC_RESOURCE_TYPE::TEXTURE,  // wind texture
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX},
      {3, GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER,  // grass config
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX |
           GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT},
      {4, GRAPHIC_RESOURCE_TYPE::TEXTURE,  // albedo texture
       GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT},
  };
  m_bindingTable = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
      descriptions, ARRAYSIZE(descriptions),
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
      "grassBindingTable");

  graphics::BindingDescription groundDescriptions[] = {
      {3, GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER,  // grass config
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX |
           GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT},
      {4, GRAPHIC_RESOURCE_TYPE::TEXTURE,  // albedo texture
       GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT},
  };
  m_groundBindingTable = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
      groundDescriptions, ARRAYSIZE(groundDescriptions),
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
      "grassGroundBindingTable");
      */
}

void GrassTechnique::render(const uint32_t id,
                            const BindingTableHandle passHandle) {
  if (id != GRASS_TECHNIQUE_FORWARD) {
    SE_CORE_WARN("Anything other than forward unsupported for grass for now");
    return;
  }
  std::vector<glm::vec3> data;
  static float t = 0.0f;
  data.push_back({0, 0, 0});
  data.push_back({0, 10, 0});
  data.push_back({0, 10, 0});
  data.push_back({0, 10, 10});
  data.push_back({0, 10, 10});
  data.push_back({10, 10, 10});
  globals::DEBUG_RENDERER->drawLines(
      &data[0].x, sizeof(glm::vec3) * data.size(), glm::vec4{1, 0, 0, 1});
  tileDebug();

  /*
  globals::CONSTANT_BUFFER_MANAGER->update(m_grassConfigHandle, &m_grassConfig);

  globals::BINDING_TABLE_MANAGER->bindBuffer(m_bindingTable,
                                             m_tilesPointsHandle, 0, 0);
  globals::BINDING_TABLE_MANAGER->bindBuffer(m_bindingTable,
                                             m_tilesIndicesHandle, 1, 1);
  globals::BINDING_TABLE_MANAGER->bindTexture(m_bindingTable, m_windTexture, 2,
                                              2, false);
  globals::BINDING_TABLE_MANAGER->bindConstantBuffer(m_bindingTable,
                                                     m_grassConfigHandle, 3, 3);
  globals::BINDING_TABLE_MANAGER->bindTexture(m_bindingTable, m_albedoTexture,
                                              4, 4, false);

  globals::PSO_MANAGER->bindPSO(m_pso);
  globals::RENDERING_CONTEXT->bindCameraBuffer(m_rs);

  if (passHandle.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->bindTable(
        PSOManager::PER_PASS_BINDING_INDEX, passHandle, m_rs);
  }
  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_OBJECT_BINDING_INDEX, m_bindingTable, m_rs);

  const int pointsPerBlade = 15;
  const int pointsPerTile = m_grassConfig.pointsPerTile;
  const int tileCount = m_grassConfig.tilesPerSide * m_grassConfig.tilesPerSide;
  globals::RENDERING_CONTEXT->renderProcedural(tileCount * pointsPerTile *
                                               pointsPerBlade);

  globals::BINDING_TABLE_MANAGER->bindConstantBuffer(m_groundBindingTable,
                                                     m_grassConfigHandle, 3, 3);
  globals::BINDING_TABLE_MANAGER->bindTexture(
      m_groundBindingTable, m_groundAlbedoTexture, 4, 4, false);
  globals::PSO_MANAGER->bindPSO(m_groundPso);
  globals::RENDERING_CONTEXT->bindCameraBuffer(m_groundRs);

  if (passHandle.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->bindTable(
        PSOManager::PER_PASS_BINDING_INDEX, passHandle, m_rs);

    globals::BINDING_TABLE_MANAGER->bindTable(
        PSOManager::PER_OBJECT_BINDING_INDEX, m_groundBindingTable, m_groundRs);

    globals::RENDERING_CONTEXT->renderProcedural(6);
  }
  */
}

void GrassTechnique::clear(const uint32_t id) {
  if (id != GRASS_TECHNIQUE_FORWARD) {
    SE_CORE_WARN("Anything other than forward unsupported for grass for now");
    return;
  }
  if (m_grassBuffer.isHandleValid()) {
    globals::BUFFER_MANAGER->free(m_grassBuffer);
    m_grassBuffer = {0};
  }
  if (m_windTexture.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(m_windTexture);
    m_windTexture = {0};
  }
  if (m_albedoTexture.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(m_albedoTexture);
    m_albedoTexture = {0};
  }
  if (m_groundAlbedoTexture.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(m_groundAlbedoTexture);
    m_groundAlbedoTexture = {0};
  }
  if (m_tilesPointsHandle.isHandleValid()) {
    globals::BUFFER_MANAGER->free(m_tilesPointsHandle);
    m_tilesPointsHandle = {0};
  }
  if (m_tilesIndicesHandle.isHandleValid()) {
    globals::BUFFER_MANAGER->free(m_tilesIndicesHandle);
    m_tilesIndicesHandle = {0};
  }
  if (m_bindingTable.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->free(m_bindingTable);
    m_bindingTable = {0};
  }
  if (m_groundBindingTable.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->free(m_groundBindingTable);
    m_groundBindingTable = {0};
  }
}

void GrassTechnique::tileDebug() {

  m_tiles.clear();
  m_tiles.reserve(MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE);
  // memset(m_tiles.data(), 0,
  //       MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE * sizeof(BoundingBox));
  // let us being by computing the tiles bounding boxes
  float halfSize = (static_cast<float>(m_grassConfig.tilesPerSide) / 2.0f) *
                   m_grassConfig.tileSize;
  glm::vec3 minCorner =
      m_grassConfig.gridOrigin - glm::vec3{halfSize, 0.0f, halfSize};

  float tw = m_grassConfig.tileSize;
  for (int tileY = 0; tileY < m_grassConfig.tilesPerSide; ++tileY) {
    for (int tileX = 0; tileX < m_grassConfig.tilesPerSide; ++tileX) {
      BoundingBox box;
      box.min = minCorner + glm::vec3{tw * tileX, 0, tw * tileY};
      box.max = minCorner + glm::vec3{tw * (tileX + 1), 0.1f, tw * (tileY + 1)};
      m_tiles.emplace_back(box);
    }
  }
  globals::DEBUG_RENDERER->drawBoundingBoxes(
      m_tiles.data(), m_grassConfig.tilesPerSide * m_grassConfig.tilesPerSide,
      glm::vec4(1, 0, 0, 1));

  m_grassConfigOld = m_grassConfig;
}
}  // namespace SirEngine::graphics
