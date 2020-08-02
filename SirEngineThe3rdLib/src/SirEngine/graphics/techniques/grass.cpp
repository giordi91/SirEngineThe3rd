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

void GrassTechnique::setup() {
  SE_CORE_INFO("Setup grass");

  m_rs = globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(GRASS_RS);
  m_pso = globals::PSO_MANAGER->getHandleFromName(GRASS_PSO);

  // lets read the grass file
  const char *grassFile = "../data/external/grass/pointsOld.json";
  const char *grassFile2 = "../data/processed/grass/grass.points";
  auto jobj = getJsonObj(grassFile);

  readAllBytes(grassFile2, m_binaryData);

  const auto *const mapper =
      getMapperData<PointTilerMapperData>(m_binaryData.data());
  const auto *nameData = reinterpret_cast<const char *>(
      m_binaryData.data() + sizeof(BinaryFileHeader));
  auto *pointData = reinterpret_cast<float *>(
      m_binaryData.data() + sizeof(BinaryFileHeader) + mapper->nameSizeInByte);
  m_grassConfig.tilesPerSide = 3;
  m_grassConfig.tileSize = 15;
  m_grassConfig.sourceDataTileCount = mapper->tileCount;
  // used to find changes in config
  m_grassConfigOld = m_grassConfig;
  globals::INTEROP_DATA->registerData("grassConfig", &m_grassConfig);

  std::vector<BoundingBox> tiles;
  tiles.reserve(MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE);
  memset(tiles.data(), 0,
         MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE * sizeof(BoundingBox));
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
      tiles.emplace_back(box);
      m_tilesPoints.push_back(box.min);
    }
  }

  int runtimeTilesCount =
      m_grassConfig.tilesPerSide * m_grassConfig.tilesPerSide;
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

  m_debugHandle = globals::DEBUG_RENDERER->drawBoundingBoxes(
      tiles.data(), MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE,
      glm::vec4(1, 0, 0, 1), "debugGrassTiles");

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
  graphics::BindingDescription descriptions[4] = {
      {0, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,  // tiles points
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX},
      {1, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,  // tiles ids
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX},
      {2, GRAPHIC_RESOURCE_TYPE::TEXTURE,  // wind texture
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX},
      {3, GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER,  // grass config
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX},
  };
  m_bindingTable = globals::BINDING_TABLE_MANAGER->allocateBindingTable(
      descriptions, 4,
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
      "grassBindingTable");

  /*
  // OLD stuff
  const uint32_t tileCount = jobj.size();
  assert((tileCount > 0) && "no tiles found in the file");
  // assuming all tiles have same size
  assertInJson(jobj[0], "points");
  const uint32_t pointCount = jobj[0]["points"].size();
  const uint64_t totalSize = sizeof(float) * 4 * pointCount * tileCount;

  auto *data =
      static_cast<float *>(globals::FRAME_ALLOCATOR->allocate(totalSize));

  auto *aabbs = static_cast<BoundingBox *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(BoundingBox) * tileCount));

  // looping the tiles
  int counter = 0;
  int tileCounter = 0;
  for (const auto &tile : jobj) {
    assertInJson(tile, "aabb");
    assertInJson(tile, "points");
    const auto &aabbJ = tile["aabb"];
    const glm::vec3 minAABB(aabbJ[0][0].get<float>(), aabbJ[0][1].get<float>(),
                            aabbJ[0][2].get<float>());
    const glm::vec3 maxAABB(aabbJ[1][0].get<float>(), aabbJ[1][1].get<float>(),
                            aabbJ[1][2].get<float>());
    aabbs[tileCounter].min = minAABB;
    aabbs[tileCounter].max = maxAABB;

    const auto &points = tile["points"];
    uint32_t currentPointCount = points.size();
    assert(currentPointCount == pointCount);

    for (uint32_t i = 0; i < currentPointCount; ++i) {
      data[counter++] = points[i][0].get<float>();
      data[counter++] = points[i][1].get<float>();
      data[counter++] = points[i][2].get<float>();
      data[counter++] = 1;
    }
    tileCounter++;
  }


  // we have the tiles, good now we want to render the blades
  m_grassBuffer = globals::BUFFER_MANAGER->allocate(
      totalSize, data, "grassBuffer", pointCount * tileCount, sizeof(float) * 4,
      BufferManager::STORAGE_BUFFER);
  // setup a material

  const char *queues[5] = {"grassForward", nullptr, nullptr, nullptr, nullptr};

  m_grassMaterial = globals::MATERIAL_MANAGER->allocateMaterial(
      "grassMaterial", MaterialManager::ALLOCATE_MATERIAL_FLAG_BITS::NONE,

      queues);
  globals::MATERIAL_MANAGER->bindBuffer(m_grassMaterial, m_grassBuffer, 0,
                                        SHADER_QUEUE_FLAGS::FORWARD);

  globals::MATERIAL_MANAGER->bindTexture(m_grassMaterial, m_windTexture, 1, 1,
                                         SHADER_QUEUE_FLAGS::FORWARD, false);

  // lets create the needed stuff to add the object to the queue
  RenderableDescription description{};
  description.buffer = m_grassBuffer;
  description.subranges[0].m_offset = 0;
  description.subranges[0].m_size = totalSize;
  description.subragesCount = 1;

  description.materialHandle = m_grassMaterial;
  //*3 because we render a triangle for now
  int verticesPerTriangle = 3;
  int trianglesPerBlade = 5;
  // description.primitiveToRender = 15;
  description.primitiveToRender =
      pointCount * tileCount * verticesPerTriangle * trianglesPerBlade;
  globals::RENDERING_CONTEXT->addRenderablesToQueue(description);
  */
}

void GrassTechnique::render(const BindingTableHandle passHandle) {
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

  globals::PSO_MANAGER->bindPSO(m_pso);
  globals::RENDERING_CONTEXT->bindCameraBuffer(m_rs);

  if (passHandle.isHandleValid()) {
    globals::BINDING_TABLE_MANAGER->bindTable(
        PSOManager::PER_PASS_BINDING_INDEX, passHandle, m_rs);
  }
  globals::BINDING_TABLE_MANAGER->bindTable(
      PSOManager::PER_OBJECT_BINDING_INDEX, m_bindingTable, m_rs);

  const int pointsPerBlade = 15;
  const int pointsPerTile = 500;
  const int tileCount = m_grassConfig.tilesPerSide * m_grassConfig.tilesPerSide;
  globals::RENDERING_CONTEXT->renderProcedural(tileCount * pointsPerTile *
                                               pointsPerBlade);
}

void GrassTechnique::clear() {
  if (m_grassBuffer.isHandleValid()) {
    globals::BUFFER_MANAGER->free(m_grassBuffer);
    m_grassBuffer = {0};
  }
  if (m_windTexture.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(m_windTexture);
    m_windTexture = {0};
  }
  if (m_debugHandle.isHandleValid()) {
    globals::DEBUG_RENDERER->free(m_debugHandle);
    m_debugHandle = {0};
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
}

void GrassTechnique::tileDebug() {
  bool grassTileSizeChanged =
      abs(m_grassConfig.tileSize - m_grassConfigOld.tileSize) > 0.001;
  bool grassTileCountChanged =
      m_grassConfig.tilesPerSide != m_grassConfigOld.tilesPerSide;
  if (grassTileSizeChanged | grassTileCountChanged) {
    std::vector<BoundingBox> tiles;
    tiles.reserve(MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE);
    memset(tiles.data(), 0,
           MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE * sizeof(BoundingBox));
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
        box.max =
            minCorner + glm::vec3{tw * (tileX + 1), 0.1f, tw * (tileY + 1)};
        tiles.emplace_back(box);
      }
    }

    globals::DEBUG_RENDERER->updateBoundingBoxesData(
        m_debugHandle, tiles.data(), MAX_GRASS_PER_SIDE * MAX_GRASS_PER_SIDE);

    m_grassConfigOld = m_grassConfig;

    // globals::CONSTANT_BUFFER_MANAGER->
  }
}
}  // namespace SirEngine::graphics
