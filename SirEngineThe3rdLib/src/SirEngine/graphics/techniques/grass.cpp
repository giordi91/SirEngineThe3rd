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
      {6, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,  // lod buffer
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX},
  };

  for (int i = 0; i < 4; ++i) {
    m_bindingTable[i] = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
        descriptions, ARRAYSIZE(descriptions),
        graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
        "grassBindingTable");
  }

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

  graphics::BindingDescription clearDescription[2] = {
      {0, GRAPHIC_RESOURCE_TYPE::READWRITE_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_COMPUTE},
      {1, GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_COMPUTE},
  };
  m_clearBindingTable = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
      clearDescription, 2,
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
      "clearBindingTable");
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
  m_grassConfig.lodThresholds = {40, 70, 110, 0};
  m_grassConfig.pointsPerTileLod= {500, 300, 200, 100};

  m_rs = globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(GRASS_RS);
  m_pso = globals::PSO_MANAGER->getHandleFromName(GRASS_PSO);

  m_groundRs =
      globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(GRASS_PLANE_RS);
  m_groundPso = globals::PSO_MANAGER->getHandleFromName(GRASS_PLANE_PSO);
  m_grassCullScanRs =
      globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(GRASS_CULL_SCAN_RS);
  m_grassCullScanPso =
      globals::PSO_MANAGER->getHandleFromName(GRASS_CULL_SCAN_PSO);
  m_grassClearRs =
      globals::ROOT_SIGNATURE_MANAGER->getHandleFromName("grassClearRS");
  m_grassClearPso = globals::PSO_MANAGER->getHandleFromName("grassClearPSO");

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
      sizeof(uint32_t) * runtimeTilesCount + 64, m_tilesIndices.data(),
      "grassTilesIndicesBuffer", runtimeTilesCount, sizeof(uint32_t),
      BufferManager::STORAGE_BUFFER);
  m_grassConfigHandle = globals::CONSTANT_BUFFER_MANAGER->allocate(
      sizeof(m_grassConfig),
      ConstantBufferManager::CONSTANT_BUFFER_FLAG_BITS::UPDATED_EVERY_FRAME,
      &m_grassConfig);

  buildBindingTables();

  m_cullingInBuffer = globals::BUFFER_MANAGER->allocate(
      sizeof(glm::vec4) * runtimeTilesCount + 64, m_tilePositions.data(),
      "cullingInput", runtimeTilesCount, sizeof(glm::vec4),
      BufferManager::BUFFER_FLAGS_BITS::GPU_ONLY |
          BufferManager::BUFFER_FLAGS_BITS::STORAGE_BUFFER);

  static constexpr uint32_t SUPPORT_DATA_OFFSET = 16;
  // multiplying by 4 because we have 4 lods
  int worstCaseNumberOfTiles = runtimeTilesCount * 4;
  // five is from the fact we have 4 lods atomic spaced out and our indirect
  // buffers also to note we are using size of
  int supportData = SUPPORT_DATA_OFFSET * 5;
  m_outTiles = globals::BUFFER_MANAGER->allocate(
      sizeof(uint64_t) * (supportData + worstCaseNumberOfTiles), nullptr,
      "tilesOutput", runtimeTilesCount, sizeof(int),
      BufferManager::BUFFER_FLAGS_BITS::GPU_ONLY |
          BufferManager::BUFFER_FLAGS_BITS::STORAGE_BUFFER |
          BufferManager::BUFFER_FLAGS_BITS::RANDOM_WRITE |
          BufferManager::BUFFER_FLAGS_BITS::INDIRECT_BUFFER);

  // 4 bindings for the LODs
  for (uint32_t i = 0; i < 4; ++i) {
    m_lodBuffer[i] = globals::BUFFER_MANAGER->allocate(
        sizeof(int32_t), &i, "lodBuffer", 1, sizeof(int),
        BufferManager::BUFFER_FLAGS_BITS::GPU_ONLY |
            BufferManager::BUFFER_FLAGS_BITS::STORAGE_BUFFER);
  }

  // binding
  globals::BINDING_TABLE_MANAGER->bindBuffer(m_scanBindingTable,
                                             m_cullingInBuffer, 0, 0);
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
        PSOManager::PER_PASS_BINDING_INDEX, passHandle, m_groundRs);
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

  for (int i = 0; i < 4; ++i) {
    globals::BINDING_TABLE_MANAGER->bindBuffer(m_bindingTable[i],
                                               m_tilesPointsHandle, 0, 0);
    globals::BINDING_TABLE_MANAGER->bindBuffer(m_bindingTable[i],
                                               m_tilesIndicesHandle, 1, 1);
    globals::BINDING_TABLE_MANAGER->bindTexture(m_bindingTable[i],
                                                m_windTexture, 2, 2, false);
    globals::BINDING_TABLE_MANAGER->bindConstantBuffer(
        m_bindingTable[i], m_grassConfigHandle, 3, 3);
    globals::BINDING_TABLE_MANAGER->bindTexture(m_bindingTable[i],
                                                m_albedoTexture, 4, 4, false);
    globals::BINDING_TABLE_MANAGER->bindBuffer(m_bindingTable[i], m_outTiles, 5,
                                               5);
    globals::BINDING_TABLE_MANAGER->bindBuffer(m_bindingTable[i],
                                               m_lodBuffer[i], 6, 6);

    globals::BINDING_TABLE_MANAGER->bindTable(
        PSOManager::PER_OBJECT_BINDING_INDEX, m_bindingTable[i], m_rs);
    globals::PSO_MANAGER->bindPSO(m_pso);
    globals::RENDERING_CONTEXT->bindCameraBuffer(m_rs);

    if (passHandle.isHandleValid()) {
      globals::BINDING_TABLE_MANAGER->bindTable(
          PSOManager::PER_PASS_BINDING_INDEX, passHandle, m_rs);
    }

    globals::RENDERING_CONTEXT->renderProceduralIndirect(m_outTiles,i*sizeof(int)*4);
  }

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
  if (m_outTiles.isHandleValid()) {
    globals::BUFFER_MANAGER->free(m_outTiles);
    m_outTiles = {0};
  }
  if (m_lodBuffer[0].isHandleValid()) {
    for (int i = 0; i < 4; ++i) {
      globals::BUFFER_MANAGER->free(m_lodBuffer[i]);
      m_lodBuffer[i] = {0};
    }
  }
  if (m_bindingTable[0].isHandleValid()) {
    for (int i = 0; i < 4; ++i) {
      globals::BINDING_TABLE_MANAGER->free(m_bindingTable[i]);
      m_bindingTable[i] = {0};
    }
  }
  if (m_groundBindingTable.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->free(m_groundBindingTable);
    m_groundBindingTable = {0};
  }
  if (m_scanBindingTable.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->free(m_scanBindingTable);
    m_scanBindingTable = {0};
  }
  if (m_clearBindingTable.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->free(m_clearBindingTable);
    m_clearBindingTable = {0};
  }

}

void GrassTechnique::prePassRender(uint32_t) { performCulling(); }

void GrassTechnique::performCulling() {
  // here we compute the surviving tiles

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
  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_OBJECT_BINDING_INDEX, m_scanBindingTable,
      m_grassCullScanRs, true);
  globals::RENDERING_CONTEXT->bindCameraBuffer(m_grassCullScanRs, true);

  int totalTiles = m_grassConfig.tilesPerSide * m_grassConfig.tilesPerSide;
  int groupSize = 64;
  int groupX = totalTiles % groupSize == 0 ? totalTiles / groupSize
                                           : totalTiles / groupSize + 1;
  globals::RENDERING_CONTEXT->dispatchCompute(groupX, 1, 1);

  globals::BUFFER_MANAGER->transitionBuffer(
      m_outTiles,
      {
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_WRITE,
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_WRITE,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_COMPUTE,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_COMPUTE,
      });

  globals::PSO_MANAGER->bindPSO(m_grassClearPso);
  globals::RENDERING_CONTEXT->bindCameraBuffer(m_grassClearRs, true);
  globals::BINDING_TABLE_MANAGER->bindBuffer(m_clearBindingTable, m_outTiles, 0,
                                             0);
  globals::BINDING_TABLE_MANAGER->bindConstantBuffer(m_clearBindingTable, m_grassConfigHandle, 1,
                                             1);
  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_OBJECT_BINDING_INDEX, m_clearBindingTable, m_grassClearRs,
      true);

  globals::RENDERING_CONTEXT->dispatchCompute(1, 1, 1);

  globals::BUFFER_MANAGER->transitionBuffer(
      m_outTiles,
      {
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_WRITE,
          BufferManager::BUFFER_BARRIER_STATE_BITS::BUFFER_STATE_INDIRECT_DRAW,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_COMPUTE,
          BufferManager::BUFFER_BARRIER_STAGE_BITS::BUFFER_STAGE_INDIRECT_DRAW,
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
