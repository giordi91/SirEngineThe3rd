#include "SirEngine/graphics/debugRenderer.h"

#include "SirEngine/globals.h"
#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/cpu/resizableVector.h"
#include "SirEngine/memory/cpu/stackAllocator.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"
#include "camera.h"

namespace SirEngine {
static const char *LINE_RS = "debugDrawLinesSingleColorRS";
static const char *LINE_PSO = "debugDrawLinesSingleColorPSO";

void DebugRenderer::initialize() {
  GPUSlabAllocatorInitializeConfig config{};
  config.initialSlabs = 1;
  config.allowNewSlabAllocations = true;
  config.slabSizeInBytes = 16 * MB_TO_BYTE;
  uint32_t frames = globals::ENGINE_CONFIG->m_frameBufferingCount;
  assert(frames <= MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < frames; ++i) {
    m_lineSlab[i].initialize(config);
  }

  m_lineRS = globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(LINE_RS);
  m_linePSO = globals::PSO_MANAGER->getHandleFromName(LINE_PSO);
}
inline int push3toVec(float *data, const glm::vec4 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;

  return counter;
}
inline int push3toVec(float *data, const glm::vec3 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;

  return counter;
}

inline int push3toVec(float *data, const glm::vec3 v, int counter,
                      const glm::mat4x4 matrix) {
  glm::vec4 modified = matrix * glm::vec4{v.x, v.y, v.z, 1.0f};

  data[counter++] = modified.x;
  data[counter++] = modified.y;
  data[counter++] = modified.z;

  return counter;
}
inline int push4toVec(float *data, const glm::vec3 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;
  data[counter++] = 1.0f;

  return counter;
}
inline int push3toVec(float *data, float x, float y, float z, int counter) {
  data[counter++] = x;
  data[counter++] = y;
  data[counter++] = z;

  return counter;
}
inline int push4toVec(float *data, float x, float y, float z, int counter) {
  data[counter++] = x;
  data[counter++] = y;
  data[counter++] = z;
  data[counter++] = 1.0f;

  return counter;
}
int drawSquareBetweenTwoPoints(float *data, const glm::vec3 minP,
                               const glm::vec3 maxP, const float y,
                               int counter) {
  counter = push4toVec(data, minP.x, y, minP.z, counter);
  counter = push4toVec(data, maxP.x, y, minP.z, counter);

  counter = push4toVec(data, maxP.x, y, minP.z, counter);
  counter = push4toVec(data, maxP.x, y, maxP.z, counter);

  counter = push4toVec(data, maxP.x, y, maxP.z, counter);
  counter = push4toVec(data, minP.x, y, maxP.z, counter);

  counter = push4toVec(data, minP.x, y, maxP.z, counter);
  counter = push4toVec(data, minP.x, y, minP.z, counter);
  return counter;
}
int drawSquareBetweenTwoPointsSize3(float *data, const glm::vec3 minP,
                                    const glm::vec3 maxP, const float y,
                                    int counter) {
  counter = push3toVec(data, minP.x, y, minP.z, counter);
  counter = push3toVec(data, maxP.x, y, minP.z, counter);

  counter = push3toVec(data, maxP.x, y, minP.z, counter);
  counter = push3toVec(data, maxP.x, y, maxP.z, counter);

  counter = push3toVec(data, maxP.x, y, maxP.z, counter);
  counter = push3toVec(data, minP.x, y, maxP.z, counter);

  counter = push3toVec(data, minP.x, y, maxP.z, counter);
  counter = push3toVec(data, minP.x, y, minP.z, counter);
  return counter;
}

DebugRenderer::DebugRenderer() : m_lineBindHandles(HANDLES_INITIAL_SIZE) {}

void DebugRenderer::cleanup() {
  for (int i = 0; i < globals::ENGINE_CONFIG->m_frameBufferingCount; ++i) {
    m_lineSlab[i].cleanup();
  }

  int count = m_lineBindHandles.size();
  for (int i = 0; i < count; ++i) {
    BindingTableHandle handle{m_lineBindHandles[i]};
    if (handle.isHandleValid()) {
      globals::BINDING_TABLE_MANAGER->free(handle);
    }
  }
  m_lineBindHandles.clear();
}

void DebugRenderer::free() {}

DebugDrawHandle DebugRenderer::drawPointsUniformColor(float *data,
                                                      uint32_t sizeInByte,
                                                      glm::vec4 color,
                                                      float size,
                                                      const char *debugName) {
  assert(0);
  return {};
}

DebugDrawHandle DebugRenderer::drawSkeleton(Skeleton *skeleton, glm::vec4 color,
                                            float pointSize) {
  assert(0);
  return {};
}

void DebugRenderer::render(TextureHandle input, TextureHandle depth) {
  // draw lines
  int slabCount = m_lineSlab[globals::CURRENT_FRAME].getSlabCount();
  assureLinesTables(slabCount);

  for (int i = 0; i < slabCount; ++i) {
    uint32_t allocatedSize =
        m_lineSlab[globals::CURRENT_FRAME].getAllocatedBytes(i);
    assert((allocatedSize % 8 == 0));
    uint32_t primCount = allocatedSize / (sizeof(float) * 8);
    if (primCount == 0) {
      continue;
    }

    BufferHandle bhandle =
        m_lineSlab[globals::CURRENT_FRAME].getBufferHandle(i);
    const auto bindHandle = BindingTableHandle{m_lineBindHandles[i]};
    globals::BINDING_TABLE_MANAGER->bindBuffer(bindHandle, bhandle, 0, 0);
    globals::RENDERING_CONTEXT->bindCameraBuffer(m_lineRS);
    globals::PSO_MANAGER->bindPSO(m_linePSO);
    globals::BINDING_TABLE_MANAGER->bindTable(3, bindHandle, m_lineRS);

    auto w = static_cast<float>(globals::ENGINE_CONFIG->m_windowWidth);
    auto h = static_cast<float>(globals::ENGINE_CONFIG->m_windowHeight);

    globals::RENDERING_CONTEXT->setViewportAndScissor(0, 0, w, h, 0, 1.0f);
    globals::RENDERING_CONTEXT->renderProcedural(primCount);
  }
}

void DebugRenderer::drawBoundingBoxes(const BoundingBox *data, const int count,
                                      const glm::vec4 color) {
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4
  // vertical two is because we need two points per line, we are not doing
  // line-strip
  const int totalSize = 3 * count * 12 * 2;  // here 3 is the xmfloat4

  auto *points = static_cast<float *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec3) * count * 12 * 2));
  int counter = 0;
  for (int i = 0; i < count; ++i) {
    assert(counter <= totalSize);
    const auto &minP = data[i].min;
    const auto &maxP = data[i].max;
    counter =
        drawSquareBetweenTwoPointsSize3(points, minP, maxP, minP.y, counter);
    counter =
        drawSquareBetweenTwoPointsSize3(points, minP, maxP, maxP.y, counter);

    // draw vertical lines
    counter = push3toVec(points, minP, counter);
    counter = push3toVec(points, minP.x, maxP.y, minP.z, counter);
    counter = push3toVec(points, maxP.x, minP.y, minP.z, counter);
    counter = push3toVec(points, maxP.x, maxP.y, minP.z, counter);

    counter = push3toVec(points, maxP.x, minP.y, maxP.z, counter);
    counter = push3toVec(points, maxP.x, maxP.y, maxP.z, counter);

    counter = push3toVec(points, minP.x, minP.y, maxP.z, counter);
    counter = push3toVec(points, minP.x, maxP.y, maxP.z, counter);
    assert(counter <= totalSize);
  }
  return drawLines(points, totalSize * sizeof(float), color);
}

DebugDrawHandle DebugRenderer::drawAnimatedBoundingBoxes(
    DebugDrawHandle handle, BoundingBox *data, int count, glm::vec4 color,
    const char *debugName) {
  assert(0);
  return {};
}

DebugDrawHandle DebugRenderer::drawAnimatedBoundingBoxFromFullPoints(
    const DebugDrawHandle handle, const glm::vec3 *data, const int count,
    const glm::vec4 color, const char *debugName) {
  assert(0);
  return {};
}

DebugDrawHandle DebugRenderer::drawMatrix(const glm::mat4 &mat, float size,
                                          glm::vec4 color,
                                          const char *debugName) {
  assert(0);
  return {};
}

void DebugRenderer::drawLines(float *data, const uint32_t sizeInByte,
                              const glm::vec4 color) {
  // making sure is a multiple of 3, float3 one per point
  assert((sizeInByte % (sizeof(float) * 3) == 0));
  uint32_t count = sizeInByte / (sizeof(float) * 3);

  // TODO we are going to allocate twice the amount of data, since we are going
  // to use a float4s one for position and one for colors, this might be
  // optimized to float3 but needs to be careful
  // https://giordi91.github.io/post/spirvvec3/
  uint32_t finalSize = count * 2 * sizeof(float) * 4;
  auto *paddedData =
      static_cast<float *>(globals::FRAME_ALLOCATOR->allocate(finalSize));

  for (int i = 0; i < count; ++i) {
    paddedData[i * 8 + 0] = data[i * 3 + 0];
    paddedData[i * 8 + 1] = data[i * 3 + 1];
    paddedData[i * 8 + 2] = data[i * 3 + 2];
    paddedData[i * 8 + 3] = 1.0f;

    paddedData[i * 8 + 4] = color.x;
    paddedData[i * 8 + 5] = color.y;
    paddedData[i * 8 + 6] = color.z;
    paddedData[i * 8 + 7] = color.w;
  }

  // ignoring the handle we are going to flush every frame
  // in the future we can use a stack allocator which would be more suitable
  // but the slab allocator was the best bang for the buck, since can be re-used
  // in many places
  m_lineSlab[globals::CURRENT_FRAME].allocate(finalSize, paddedData);
}

void DebugRenderer::assureLinesTables(const int slabCount) {
  // init binding table
  graphics::BindingDescription descriptions[1] = {
      {0, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX}};

  int count = m_lineBindHandles.size();
  for (int i = count; i < slabCount; ++i) {
    BindingTableHandle handle =
        globals::BINDING_TABLE_MANAGER->allocateBindingTable(
            descriptions, 1,
            graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
            "debugLines");
    m_lineBindHandles.pushBack(handle.handle);
  }
}

void DebugRenderer::newFrame() { m_lineSlab[globals::CURRENT_FRAME].clear(); }

void DebugRenderer::drawCamera(const CameraController *camera,
                               glm::vec4 color) {
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4
  // vertical two is because we need two points per line, we are not doing
  // line-strip
  int count = 2;
  int linesPerBox = 12;
  auto *points = static_cast<float *>(globals::FRAME_ALLOCATOR->allocate(
      sizeof(glm::vec3) * count * linesPerBox * 2));
  int counter = 0;
  const auto &minP = glm::vec3(0, 0, 0);
  const auto &maxP = glm::vec3(10, 10, 10);

  glm::mat4x4 cameraM = camera->getViewInverse(glm::mat4x4(1.0f));
  // frustum
  const CameraBuffer &buffer = camera->getCameraBuffer();
  float angle = camera->getVfov();

  // NOTE at one point this might be something that the camera decides?
  float aspsectRatio =
      static_cast<float>(globals::ENGINE_CONFIG->m_windowWidth) /
      static_cast<float>(globals::ENGINE_CONFIG->m_windowWidth);
  float apiCompensateFactor =  globals::ENGINE_CONFIG->m_graphicsAPI==GRAPHIC_API::DX12? -1 : 1;
  float nnear = camera->getNear()*apiCompensateFactor ;
  float hnear = 2.0f * tanf(angle / 2.0f) * nnear;
  float wnear = hnear * aspsectRatio;

  float ffar = camera->getFar()*apiCompensateFactor;
  float hfar = 2.0f * tanf(angle / 2.0f) * ffar;
  float wfar = hfar * aspsectRatio;

  float hnearhalf = hnear / 2.0f;
  float wnearhalf = wnear / 2.0f;

  float hfarhalf = hfar / 2.0f;
  float wfarhalf = wfar / 2.0f;

  // near plane
  counter =
      push3toVec(points, glm::vec3{wnearhalf, -hnearhalf, -nnear}, counter,cameraM);
  counter =
      push3toVec(points, glm::vec3{wnearhalf, hnearhalf, -nnear}, counter,cameraM);

  counter =
      push3toVec(points, glm::vec3{wnearhalf, hnearhalf, -nnear}, counter,cameraM);
  counter =
      push3toVec(points, glm::vec3{-wnearhalf, hnearhalf, -nnear}, counter,cameraM);

  counter =
      push3toVec(points, glm::vec3{-wnearhalf, hnearhalf, -nnear}, counter,cameraM);
  counter =
      push3toVec(points, glm::vec3{-wnearhalf, -hnearhalf, -nnear}, counter,cameraM);

  counter =
      push3toVec(points, glm::vec3{-wnearhalf, -hnearhalf, -nnear}, counter,cameraM);
  counter =
      push3toVec(points, glm::vec3{wnearhalf, -hnearhalf, -nnear}, counter,cameraM);

  // far plane
  counter = push3toVec(points, glm::vec3{wfarhalf, -hfarhalf, -ffar}, counter,cameraM);
  counter = push3toVec(points, glm::vec3{wfarhalf, hfarhalf, -ffar}, counter,cameraM);

  counter = push3toVec(points, glm::vec3{wfarhalf, hfarhalf, -ffar}, counter,cameraM);
  counter = push3toVec(points, glm::vec3{-wfarhalf, hfarhalf, -ffar}, counter,cameraM);

  counter = push3toVec(points, glm::vec3{-wfarhalf, hfarhalf, -ffar}, counter,cameraM);
  counter = push3toVec(points, glm::vec3{-wfarhalf, -hfarhalf, -ffar}, counter,cameraM);

  counter = push3toVec(points, glm::vec3{-wfarhalf, -hfarhalf, -ffar}, counter,cameraM);
  counter = push3toVec(points, glm::vec3{wfarhalf, -hfarhalf, -ffar}, counter,cameraM);

  // sides
  counter = push3toVec(points, glm::vec3{wfarhalf, hfarhalf, -ffar}, counter,cameraM);
  counter =
      push3toVec(points, glm::vec3{wnearhalf, hnearhalf, -nnear}, counter,cameraM);

  counter = push3toVec(points, glm::vec3{wfarhalf, -hfarhalf, -ffar}, counter,cameraM);
  counter =
      push3toVec(points, glm::vec3{wnearhalf, -hnearhalf, -nnear}, counter,cameraM);

  counter = push3toVec(points, glm::vec3{-wfarhalf, hfarhalf, -ffar}, counter,cameraM);
  counter =
      push3toVec(points, glm::vec3{-wnearhalf, hnearhalf, -nnear}, counter,cameraM);

  counter = push3toVec(points, glm::vec3{-wfarhalf, -hfarhalf, -ffar}, counter,cameraM);
  counter =
      push3toVec(points, glm::vec3{-wnearhalf, -hnearhalf, -nnear}, counter,cameraM);

  return drawLines(points, counter * sizeof(float), color);
}
}  // namespace SirEngine
