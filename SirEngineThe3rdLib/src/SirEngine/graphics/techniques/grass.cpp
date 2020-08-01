#include "SirEngine/graphics/techniques/grass.h"

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/bufferManager.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/debugRenderer.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/interopData.h"
#include "SirEngine/log.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/textureManager.h"

namespace SirEngine::graphics {
void GrassTechnique::setup() {
  SE_CORE_INFO("Setup grass");

  // lets read the grass file
  const char *grassFile = "../data/external/grass/pointsOld.json";
  const char *grassFile2 = "../data/processed/grass/grass.points";
  auto jobj = getJsonObj(grassFile);

  std::vector<char> binaryData;
  readAllBytes(grassFile2, binaryData);

  const auto *const mapper =
      getMapperData<PointTilerMapperData>(binaryData.data());
  const auto *nameData = reinterpret_cast<const char *>(
      binaryData.data() + sizeof(BinaryFileHeader));
  const auto *pointData = reinterpret_cast<const float *>(
      binaryData.data() + sizeof(BinaryFileHeader) + mapper->nameSizeInByte);
  m_grassConfig.tilesPerSide = 3;
  m_grassConfig.tileSize = 15;
  m_grassConfigOld = m_grassConfig;
  globals::INTEROP_DATA->registerData("grassConfig", &m_grassConfig);

  std::vector<BoundingBox> tiles;
  tiles.reserve(maxGrassPerSide * maxGrassPerSide);
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
    }
  }

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

  m_debugHandle = globals::DEBUG_RENDERER->drawBoundingBoxes(
      tiles.data(), maxGrassPerSide * maxGrassPerSide, glm::vec4(1, 0, 0, 1),
      "debugGrassTiles");

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

  m_windTexture = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/grass/wind.texture");
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
}

void GrassTechnique::render(const BindingTableHandle passHandle) {
    tileDebug();
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
}

void GrassTechnique::tileDebug() {
  bool grassTileSizeChanged =
      abs(m_grassConfig.tileSize - m_grassConfigOld.tileSize) > 0.001;
  bool grassTileCountChanged =
      m_grassConfig.tilesPerSide != m_grassConfigOld.tilesPerSide;
  if (grassTileSizeChanged | grassTileCountChanged) {
    std::vector<BoundingBox> tiles;
    tiles.reserve(maxGrassPerSide * maxGrassPerSide);
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
        m_debugHandle, tiles.data(),
        m_grassConfig.tilesPerSide * m_grassConfig.tilesPerSide);

    m_grassConfigOld = m_grassConfig;
  }
}
}  // namespace SirEngine::graphics
