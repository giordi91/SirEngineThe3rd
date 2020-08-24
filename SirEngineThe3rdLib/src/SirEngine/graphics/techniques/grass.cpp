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
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"
#include "SirEngine/textureManager.h"

namespace SirEngine::graphics {

static const char *GRASS_RS = "grassForwardRS";
static const char *GRASS_PSO = "grassForwardPSO";
static const char *GRASS_PLANE_RS = "grassPlaneRS";
static const char *GRASS_PLANE_PSO = "grassPlanePSO";

static const char *GRASS_CULL_RS = "grassCullingRS";
static const char *GRASS_CULL_PSO = "grassCullingPSO";
static const char *GRASS_CULL_SCAN_RS = "grassCullingScanRS";
static const char *GRASS_CULL_SCAN_PSO = "grassCullingScanPSO";

void GrassTechnique::buildBindingTables() {
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
      {5, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,  // tiles ids
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX},
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

  graphics::BindingDescription cullingDescription[3] = {
      {0, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_COMPUTE},
      {1, GRAPHIC_RESOURCE_TYPE::READWRITE_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_COMPUTE},
      {2, GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_COMPUTE}};
  m_cullinngBindingTable = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
      cullingDescription, 3,
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
      "grassCullingBindingTable");

  graphics::BindingDescription scanDescription[4] = {
      {0, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_COMPUTE},
      {1, GRAPHIC_RESOURCE_TYPE::READWRITE_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_COMPUTE},
      {2, GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_COMPUTE},
      {3, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_COMPUTE},
  };
  m_scanBindingTable = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
      scanDescription, 4,
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_NONE,
      "grassScanBindingTable");
}

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
  m_grassCullRs =
      globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(GRASS_CULL_RS);
  m_grassCullPso = globals::PSO_MANAGER->getHandleFromName(GRASS_CULL_PSO);
  m_grassCullScanRs =
      globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(GRASS_CULL_SCAN_RS);
  m_grassCullScanPso =
      globals::PSO_MANAGER->getHandleFromName(GRASS_CULL_SCAN_PSO);

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

  int runtimeTilesCount = MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE;
  m_tilesIndices.reserve(runtimeTilesCount);
  m_tilePositions.reserve(runtimeTilesCount);

  // computing the min corner for the grid
  int tilesPerSide = m_grassConfig.tilesPerSide;
  float halfSize = tilesPerSide * 0.5f;
  float tw = m_grassConfig.tileSize;

  glm::vec3 minCorner =
      m_grassConfig.gridOrigin - glm::vec3(halfSize, 0, halfSize) * tw;

  for (int tileY = 0; tileY < m_grassConfig.tilesPerSide; ++tileY) {
    for (int tileX = 0; tileX < m_grassConfig.tilesPerSide; ++tileX) {
      int value =
          (rand() * static_cast<int>(m_grassConfig.sourceDataTileCount) /
           RAND_MAX);
      m_tilesIndices.push_back(value);

      auto pos = minCorner + glm::vec3(tw * tileX, 0, tw * tileY);
      m_tilePositions.emplace_back(glm::vec4(pos, 1.0));
    }
  }

  // load the textures
  m_windTexture = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/grass/wind.texture");

  m_albedoTexture = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/grass/grassAlbedo.texture");

  m_groundAlbedoTexture = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/grass/grassGround.texture");

  // load the buffers
  m_tilesPointsHandle = globals::BUFFER_MANAGER->allocate(
      mapper->pointsSizeInByte, pointData, "grassBuffer",
      mapper->tileCount * mapper->pointsPerTile, sizeof(float) * 2,
      BufferManager::STORAGE_BUFFER | BufferManager::GPU_ONLY);
  m_tilesIndicesHandle = globals::BUFFER_MANAGER->allocate(
      sizeof(uint32_t) * runtimeTilesCount, m_tilesIndices.data(),
      "grassTilesIndicesBuffer", runtimeTilesCount, sizeof(uint32_t),
      BufferManager::STORAGE_BUFFER);
  m_grassConfigHandle = globals::CONSTANT_BUFFER_MANAGER->allocate(
      sizeof(m_grassConfig),
      ConstantBufferManager::CONSTANT_BUFFER_FLAG_BITS::UPDATED_EVERY_FRAME,
      &m_grassConfig);

  buildBindingTables();

  m_cullingInBuffer = globals::BUFFER_MANAGER->allocate(
      sizeof(glm::vec4) * runtimeTilesCount, m_tilePositions.data(),
      "cullingInput", runtimeTilesCount, sizeof(glm::vec4),
      BufferManager::BUFFER_FLAGS_BITS::GPU_ONLY |
          BufferManager::BUFFER_FLAGS_BITS::STORAGE_BUFFER);

  std::vector<int> t;
  for (int i = 0; i < runtimeTilesCount; ++i) {
    t.push_back(i % 2);
  }
  m_cullingOutBuffer = globals::BUFFER_MANAGER->allocate(
      // sizeof(int) * runtimeTilesCount, nullptr, "cullingOutput",
      sizeof(int) * runtimeTilesCount, t.data(), "cullingOutput",
      runtimeTilesCount, sizeof(int),
      BufferManager::BUFFER_FLAGS_BITS::GPU_ONLY |
          BufferManager::BUFFER_FLAGS_BITS::STORAGE_BUFFER |
          BufferManager::BUFFER_FLAGS_BITS::RANDOM_WRITE);

  m_outTiles = globals::BUFFER_MANAGER->allocate(
      // sizeof(int) * runtimeTilesCount, nullptr, "cullingOutput",
      sizeof(int) * runtimeTilesCount, nullptr, "tilesOutput",
      runtimeTilesCount, sizeof(int),
      BufferManager::BUFFER_FLAGS_BITS::GPU_ONLY |
          BufferManager::BUFFER_FLAGS_BITS::STORAGE_BUFFER |
          BufferManager::BUFFER_FLAGS_BITS::RANDOM_WRITE);

  // binding
  globals::BINDING_TABLE_MANAGER->bindBuffer(m_scanBindingTable,
                                             m_cullingOutBuffer, 0, 0);
  globals::BINDING_TABLE_MANAGER->bindBuffer(m_scanBindingTable, m_outTiles, 1,
                                             1);
  globals::BINDING_TABLE_MANAGER->bindConstantBuffer(m_scanBindingTable,
                                                     m_grassConfigHandle, 2, 2);
  globals::BINDING_TABLE_MANAGER->bindBuffer(m_scanBindingTable,
                                             m_tilesIndicesHandle, 3, 3);
}

void GrassTechnique::renderGroundPlane(
    const BindingTableHandle passHandle) const {
  // render the ground plane
  globals::BINDING_TABLE_MANAGER->bindConstantBuffer(m_groundBindingTable,
                                                     m_grassConfigHandle, 0, 3);
  globals::BINDING_TABLE_MANAGER->bindTexture(
      m_groundBindingTable, m_groundAlbedoTexture, 1, 4, false);
  globals::PSO_MANAGER->bindPSO(m_groundPso);
  globals::RENDERING_CONTEXT->bindCameraBuffer(m_groundRs);

  if (passHandle.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->bindTable(
        PSOManager::PER_PASS_BINDING_INDEX, passHandle, m_rs);
  }
  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_OBJECT_BINDING_INDEX, m_groundBindingTable, m_groundRs);

  globals::RENDERING_CONTEXT->renderProcedural(6);
}

void GrassTechnique::passRender(const uint32_t id,
                                const BindingTableHandle passHandle) {
  if (id != GRASS_TECHNIQUE_FORWARD) {
    SE_CORE_WARN("Anything other than forward unsupported for grass for now");
    return;
  }
  tileDebug();

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
  // globals::BINDING_TABLE_MANAGER->bindBuffer(m_bindingTable,
  // m_cullingOutBuffer,
  //                                           5, 5);
  globals::BINDING_TABLE_MANAGER->bindBuffer(m_bindingTable, m_outTiles, 5, 5);

  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_OBJECT_BINDING_INDEX, m_bindingTable, m_rs);
  globals::PSO_MANAGER->bindPSO(m_pso);
  globals::RENDERING_CONTEXT->bindCameraBuffer(m_rs);

  if (passHandle.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->bindTable(
        PSOManager::PER_PASS_BINDING_INDEX, passHandle, m_rs);
  }

  const int pointsPerBlade = 15;
  const int pointsPerTile = m_grassConfig.pointsPerTile;
  const int tileCount = m_grassConfig.tilesPerSide * m_grassConfig.tilesPerSide;

  globals::RENDERING_CONTEXT->renderProcedural(tileCount * pointsPerTile *
                                               pointsPerBlade);

  renderGroundPlane(passHandle);
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
  if (m_cullingInBuffer.isHandleValid()) {
    globals::BUFFER_MANAGER->free(m_cullingInBuffer);
    m_cullingInBuffer = {0};
  }
  if (m_cullingOutBuffer.isHandleValid()) {
    globals::BUFFER_MANAGER->free(m_cullingOutBuffer);
    m_cullingOutBuffer = {0};
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
  if (m_cullinngBindingTable.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->free(m_cullinngBindingTable);
    m_cullinngBindingTable = {0};
  }
}

void GrassTechnique::prePassRender(uint32_t id) { performCulling(); }

void GrassTechnique::performCulling() {
  // here we compute the surviving tiles
  globals::BUFFER_MANAGER->transitionBuffer(
      m_cullingOutBuffer,
      {
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_READ,
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_WRITE,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_GRAPHICS,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_COMPUTE,
      });

  globals::BINDING_TABLE_MANAGER->bindBuffer(m_cullinngBindingTable,
                                             m_cullingInBuffer, 0, 0);
  globals::BINDING_TABLE_MANAGER->bindBuffer(m_cullinngBindingTable,
                                             m_cullingOutBuffer, 1, 1);
  globals::BINDING_TABLE_MANAGER->bindConstantBuffer(m_cullinngBindingTable,
                                                     m_grassConfigHandle, 2, 2);

  globals::PSO_MANAGER->bindPSO(m_grassCullPso);
  globals::RENDERING_CONTEXT->bindCameraBuffer(m_grassCullRs, true);
  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_OBJECT_BINDING_INDEX, m_cullinngBindingTable,
      m_grassCullRs, true);

  int totalTiles = m_grassConfig.tilesPerSide * m_grassConfig.tilesPerSide;
  int groupX = totalTiles % 16 == 0 ? totalTiles / 16 : totalTiles / 16 + 1;
  globals::RENDERING_CONTEXT->dispatchCompute(groupX, 1, 1);

  globals::BUFFER_MANAGER->transitionBuffer(
      m_cullingOutBuffer,
      {
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_WRITE,
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_READ,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_COMPUTE,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_GRAPHICS,
      });

  globals::BUFFER_MANAGER->transitionBuffer(
      m_outTiles,
      {
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_READ,
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_WRITE,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_GRAPHICS,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_COMPUTE,
      });

  // here we compact the surviving tiles
  globals::PSO_MANAGER->bindPSO(m_grassCullScanPso);
  globals::RENDERING_CONTEXT->bindCameraBuffer(m_grassCullScanRs, true);
  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_OBJECT_BINDING_INDEX, m_scanBindingTable,
      m_grassCullScanRs, true);

  totalTiles = m_grassConfig.tilesPerSide * m_grassConfig.tilesPerSide;
  int groupSize = 64;
  groupX = totalTiles % groupSize == 0 ? totalTiles / groupSize
                                       : totalTiles / groupSize + 1;
  globals::RENDERING_CONTEXT->dispatchCompute(groupX, 1, 1);

  globals::BUFFER_MANAGER->transitionBuffer(
      m_outTiles,
      {
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_WRITE,
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_READ,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_COMPUTE,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_GRAPHICS,
      });
}

void GrassTechnique::tileDebug() {
  m_tiles.clear();
  m_tiles.reserve(MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE);

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
