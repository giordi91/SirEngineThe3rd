#include "platform/windows/graphics/dx12/dx12DebugRenderer.h"

#include "SirEngine/animation/animationPlayer.h"
#include "SirEngine/animation/skeleton.h"
#include "SirEngine/bufferManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/dx12MaterialManager.h"

namespace SirEngine::dx12 {
void Dx12DebugRenderer::initialize() {}

void Dx12DebugRenderer::free(DebugDrawHandle handle) { assert(0); }

DebugDrawHandle Dx12DebugRenderer::drawPointsUniformColor(
    float *data, const uint32_t sizeInByte, const glm::vec4 color,
    const float size, const char *debugName) {
  // TODO : next time we draw points, we need to fix the code to go from vec3 to
  // vec4 and to bind mesh separately, as buffer and read manually, not input
  // assembler a anymore

  uint32_t index;
  Dx12DebugPrimitive &primitive = m_primitivesPool.getFreeMemoryData(index);

  // allocate vertex buffer
  assert((sizeInByte % (sizeof(float) * 3)) == 0);
  const uint32_t elementCount = sizeInByte / (sizeof(float) * 3);

  // TODO type of allocation should be dictated by wheter or not is updated on a
  // per frame basis or not
  primitive.m_bufferHandle = globals::BUFFER_MANAGER->allocateUpload(
      sizeInByte, elementCount, sizeof(float) * 3, debugName);
  void *mappedData =
      globals::BUFFER_MANAGER->getMappedData(primitive.m_bufferHandle);
  memcpy(mappedData, data, sizeInByte);
  primitive.m_sizeInByte = sizeInByte;

  // allocate constant buffer
  DebugPointsFixedColor settings{color, size};
  const ConstantBufferHandle chandle =
      globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(settings),
                                                        &settings);

  // store it such way that we can render it
  primitive.m_primitiveType = PRIMITIVE_TYPE::POINT;
  primitive.m_cbHandle = chandle;
  primitive.m_primitiveToRender = elementCount * 6;

  RenderableDescription description{};
  description.buffer = primitive.m_bufferHandle;
  description.subranges[0].m_offset = 0;
  description.subranges[0].m_size = sizeInByte;
  description.subragesCount = 1;

  const char *queues[5] = {nullptr, nullptr, nullptr, "debugPointsSingleColor",
                           nullptr};
  description.materialHandle = globals::MATERIAL_MANAGER->allocateMaterial(
      debugName, MaterialManager::ALLOCATE_MATERIAL_FLAG_BITS::NONE, queues);
  description.primitiveToRender = elementCount * 6;

  // generate handle for storing
  SHADER_QUEUE_FLAGS queue = SHADER_QUEUE_FLAGS::DEBUG;
  SHADER_TYPE_FLAGS type = SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR;
  const uint32_t storeHandle =
      static_cast<uint32_t>(queue) | (static_cast<uint32_t>(type) << 16);

  // TODO temp const cast. this is due to the fact that material system is not
  // yet as mature and polished as should be
  auto &runtime = const_cast<Dx12MaterialRuntime &>(
      dx12::MATERIAL_MANAGER->getMaterialRuntime(description.materialHandle));
  runtime.cbVirtualAddress =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(chandle);
  runtime.chandle = chandle;
  runtime.dataHandle = primitive.m_bufferHandle;
  primitive.m_magicNumber = MAGIC_NUMBER_COUNTER;

  globals::RENDERING_CONTEXT->addRenderablesToQueue(description);

  const DebugDrawHandle debugHandle{(MAGIC_NUMBER_COUNTER << 16) | index};
  ++MAGIC_NUMBER_COUNTER;

  return debugHandle;
}

DebugDrawHandle Dx12DebugRenderer::drawLinesUniformColor(
    float *data, const uint32_t sizeInByte, const glm::vec4 color,
    const float size, const char *debugName) {
  uint32_t index;
  Dx12DebugPrimitive &primitive = m_primitivesPool.getFreeMemoryData(index);

  // allocate vertex buffer
  assert((sizeInByte % (sizeof(float) * 4)) == 0);
  const uint32_t elementCount = sizeInByte / (sizeof(float) * 4);

  primitive.m_bufferHandle = globals::BUFFER_MANAGER->allocateUpload(
      sizeInByte, elementCount, sizeof(float) * 4, debugName);
  void *mappedData =
      globals::BUFFER_MANAGER->getMappedData(primitive.m_bufferHandle);
  memcpy(mappedData, data, sizeInByte);
  primitive.m_sizeInByte = sizeInByte;

  // allocate constant buffer
  DebugPointsFixedColor settings{color, size};
  const ConstantBufferHandle chandle =
      dx12::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(settings),
                                                     &settings);

  // store it such way that we can render it
  primitive.m_primitiveType = PRIMITIVE_TYPE::LINE;
  primitive.m_cbHandle = chandle;
  primitive.m_primitiveToRender = static_cast<int>(elementCount);

  RenderableDescription description{};
  description.buffer = primitive.m_bufferHandle;
  description.subranges[0].m_offset = 0;
  description.subranges[0].m_size = sizeInByte;
  description.subragesCount = 1;

  const char *queues[5] = {nullptr, nullptr, nullptr, "debugLinesSingleColor",
                           nullptr};

  description.materialHandle = globals::MATERIAL_MANAGER->allocateMaterial(
      debugName, MaterialManager::ALLOCATE_MATERIAL_FLAG_BITS::NONE, queues);
  description.primitiveToRender = elementCount;

  // generate handle for storing
  SHADER_QUEUE_FLAGS queue = SHADER_QUEUE_FLAGS::DEBUG;
  SHADER_TYPE_FLAGS type = SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR;
  const uint32_t storeHandle =
      static_cast<uint32_t>(queue) | (static_cast<uint32_t>(type) << 16);

  // TODO temp const cast
  auto &runtime = const_cast<Dx12MaterialRuntime &>(
      dx12::MATERIAL_MANAGER->getMaterialRuntime(description.materialHandle));
  runtime.cbVirtualAddress =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(chandle);
  runtime.chandle = chandle;
  runtime.dataHandle = primitive.m_bufferHandle;
  primitive.m_magicNumber = MAGIC_NUMBER_COUNTER;

  globals::RENDERING_CONTEXT->addRenderablesToQueue(description);

  const DebugDrawHandle debugHandle{(MAGIC_NUMBER_COUNTER << 16) | index};
  ++MAGIC_NUMBER_COUNTER;

  return debugHandle;
}

DebugDrawHandle Dx12DebugRenderer::drawSkeleton(Skeleton *skeleton,
                                                const glm::vec4 color,
                                                const float pointSize) {
  const ResizableVector<glm::mat4> &joints = skeleton->m_jointsWolrdInv;
  // first we need to convert the skeleton to points we can actually render
  auto *points = reinterpret_cast<glm::vec4 *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec4) * joints.size()));
  auto *lines =
      reinterpret_cast<glm::vec4 *>(globals::FRAME_ALLOCATOR->allocate(
          sizeof(glm::vec4) * joints.size() * 2));

  const ResizableVector<int> &parentIds = skeleton->m_parentIds;
  uint32_t lineCounter = 0;
  for (uint32_t i = 0; i < joints.size(); ++i) {
    const glm::mat4 &inv = joints[i];
    const glm::mat4 mat = glm::inverse(inv);

    glm::vec4 pos = mat[3];
    // DirectX::XMVECTOR scale;
    // DirectX::XMVECTOR rot;
    // DirectX::XMMatrixDecompose(&scale, &rot, &pos, mat);
    points[i] = glm::vec4(pos);

    if (parentIds[i] != -1) {
      // here we add a line from the parent to the children, might do a more
      // elaborate joint drawing one day
      lines[lineCounter] = points[parentIds[i]];
      lines[lineCounter + 1] = glm::vec4(pos);
      lineCounter += 2;
    }
  }
  const DebugDrawHandle pointsHandle =
      drawPointsUniformColor(&points[0].x, joints.size() * sizeof(glm::vec4),
                             color, pointSize, skeleton->m_name);

  const DebugDrawHandle linesHandle =
      drawLinesUniformColor(&lines[0].x, lineCounter * sizeof(glm::vec4), color,
                            pointSize, skeleton->m_name);

  // lets prepare the compound handle
  // there are two items only lines and points and the points is the first
  DebugTracker tracker{};
  tracker.m_compoundCount = 2;
  // TODO remove this naked allocation use an allocator
  // being compound we only store the compound count and handles
  tracker.m_compoundHandles = reinterpret_cast<DebugDrawHandle *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(DebugDrawHandle) * 2));
  tracker.m_compoundHandles[0] = pointsHandle;
  tracker.m_compoundHandles[1] = linesHandle;

  const uint32_t compoundBit = 1u << 31u;
  const DebugDrawHandle returnHandle{compoundBit |
                                     (MAGIC_NUMBER_COUNTER << 16) | 0};

  m_trackers.insert(returnHandle.handle, tracker);

  ++MAGIC_NUMBER_COUNTER;
  return returnHandle;
}

DebugDrawHandle Dx12DebugRenderer::drawAnimatedSkeleton(DebugDrawHandle handle,
                                                        AnimationPlayer *state,
                                                        const glm::vec4 color,
                                                        float pointSize) {
  const glm::mat4 *pose = state->getOutPose()->m_worldMat;
  const uint32_t jointCount = state->getOutPose()->m_skeleton->m_jointCount;

  auto *points = reinterpret_cast<glm::vec4 *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec4) * jointCount));
  auto *lines = reinterpret_cast<glm::vec4 *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec4) * jointCount * 2));

  uint32_t lineCounter = 0;

  for (uint32_t i = 0; i < jointCount; ++i) {
    const glm::mat4 mat = pose[i];
    glm::vec4 pos = mat[3];
    points[i] = glm::vec4(pos);

    const int parentId = state->getOutPose()->m_skeleton->m_parentIds[i];
    if (parentId != -1) {
      // here we add a line from the parent to the children, might do a more
      // elaborate joint drawing one day
      lines[lineCounter] = points[parentId];
      lines[lineCounter + 1] = glm::vec4(pos);
      lineCounter += 2;
    }
  }

  // if handle is null means we need to allocate, if not we reuse the memory
  if (handle.isHandleValid()) {
    // we need to allocate the memory
    // making sure our handle is an actual compound handle
    assert(isCompound(handle));
    // extact from the tracker map
    DebugTracker tracker{};
    const auto found = m_trackers.get(handle.handle, tracker);
    assert(found);
    assert(tracker.m_compoundCount == 2);

    // we have the tracker so we should be able to get out data
    const DebugDrawHandle pointsHandle = tracker.m_compoundHandles[0];
    const DebugDrawHandle linesHandle = tracker.m_compoundHandles[1];

    // lets get the trackers out for each one
    assertPoolMagicNumber(pointsHandle);
    uint32_t idx = getIndexFromHandle(pointsHandle);
    const Dx12DebugPrimitive &primitive = m_primitivesPool.getConstRef(idx);

    assert(primitive.m_sizeInByte == (sizeof(glm::vec4) * jointCount));
    void *mappedData =
        globals::BUFFER_MANAGER->getMappedData(primitive.m_bufferHandle);
    memcpy(mappedData, points, primitive.m_sizeInByte);

    assertPoolMagicNumber(linesHandle);
    idx = getIndexFromHandle(linesHandle);
    const Dx12DebugPrimitive &linePrimitive = m_primitivesPool.getConstRef(idx);

    assert(linePrimitive.m_sizeInByte == (sizeof(glm::vec4) * lineCounter));
    mappedData =
        globals::BUFFER_MANAGER->getMappedData(linePrimitive.m_bufferHandle);
    memcpy(mappedData, lines, linePrimitive.m_sizeInByte);

    // data is updated we are good to go, returning same handle
    // since a new one has not been allocated
    return handle;
  }
  DebugDrawHandle pointsHandle = drawPointsUniformColor(
      &points[0].x, jointCount * sizeof(glm::vec4), color, pointSize,
      state->getOutPose()->m_skeleton->m_name);

  DebugDrawHandle linesHandle =
      drawLinesUniformColor(&lines[0].x, lineCounter * sizeof(glm::vec4), color,
                            pointSize, state->getOutPose()->m_skeleton->m_name);

  // lets prepare the compound handle
  // there are two items only lines and points and the points is the first
  DebugTracker tracker{};
  tracker.m_compoundCount = 2;
  // being compound we only store the compound count and handles
  tracker.m_compoundHandles = reinterpret_cast<DebugDrawHandle *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(DebugDrawHandle) * 2));
  tracker.m_compoundHandles[0] = pointsHandle;
  tracker.m_compoundHandles[1] = linesHandle;

  const uint32_t compoundBit = 1u << 31u;
  const DebugDrawHandle returnHandle{compoundBit |
                                     (MAGIC_NUMBER_COUNTER << 16) | 0};

  m_trackers.insert(returnHandle.handle, tracker);

  ++MAGIC_NUMBER_COUNTER;

  return returnHandle;
}

void Dx12DebugRenderer::render(const TextureHandle input,
                               const TextureHandle depth) {
  const DrawCallConfig config{
      static_cast<uint32_t>(globals::ENGINE_CONFIG->m_windowWidth),
      static_cast<uint32_t>(globals::ENGINE_CONFIG->m_windowHeight), 0};
  globals::RENDERING_CONTEXT->renderQueueType(config,
                                              SHADER_QUEUE_FLAGS::DEBUG);
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
inline int push4toVec(float *data, const glm::vec3 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;
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

DebugDrawHandle Dx12DebugRenderer::drawBoundingBoxes(const BoundingBox *data,
                                                     int count,
                                                     const glm::vec4 color,
                                                     const char *debugName) {
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4 vertical
  // two is because we need two points per line, we are not doing trianglestrip
  int totalSize = 4 * count * 12 * 2;  // here 4 is the xmfloat4

  auto *points = reinterpret_cast<float *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec4) * count * 12 * 2));
  int counter = 0;
  for (int i = 0; i < count; ++i) {
    assert(counter <= totalSize);
    const auto &minP = data[i].min;
    const auto &maxP = data[i].max;
    counter = drawSquareBetweenTwoPoints(points, minP, maxP, minP.y, counter);
    counter = drawSquareBetweenTwoPoints(points, minP, maxP, maxP.y, counter);

    // draw vertical lines
    counter = push4toVec(points, minP, counter);
    counter = push4toVec(points, minP.x, maxP.y, minP.z, counter);
    counter = push4toVec(points, maxP.x, minP.y, minP.z, counter);
    counter = push4toVec(points, maxP.x, maxP.y, minP.z, counter);

    counter = push4toVec(points, maxP.x, minP.y, maxP.z, counter);
    counter = push4toVec(points, maxP.x, maxP.y, maxP.z, counter);

    counter = push4toVec(points, minP.x, minP.y, maxP.z, counter);
    counter = push4toVec(points, minP.x, maxP.y, maxP.z, counter);
    assert(counter <= totalSize);
  }
  drawLinesUniformColor(points, totalSize * sizeof(float), color,
                        static_cast<float>(totalSize), debugName);

  // this is not compound;
  const int compoundBit = 0;
  const DebugDrawHandle returnHandle{compoundBit |
                                     (MAGIC_NUMBER_COUNTER << 16) | 0};
  ++MAGIC_NUMBER_COUNTER;
  return returnHandle;
}

DebugDrawHandle Dx12DebugRenderer::drawAnimatedBoundingBoxes(
    DebugDrawHandle handle, BoundingBox *data, int count, glm::vec4 color,
    const char *debugName) {
  // first get AABB data
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4 vertical
  // two is because we need two points per line, we are not doing trianglestrip
  int totalSize = 3 * count * 12 * 2;  // here 3 is the xmfloat3

  auto *points = reinterpret_cast<float *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec3) * count * 12 * 2));
  int counter = 0;
  for (int i = 0; i < count; ++i) {
    assert(counter <= totalSize);
    const auto &minP = data[i].min;
    const auto &maxP = data[i].max;
    counter = drawSquareBetweenTwoPoints(points, minP, maxP, minP.y, counter);
    counter = drawSquareBetweenTwoPoints(points, minP, maxP, maxP.y, counter);

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

  assert(0);
  /*
  if (handle.isHandleValid()) {
    // lets get the trackers out for each one
    const auto found = m_trackers.find(handle.handle);
    assert(found != m_trackers.end());
    assert(found->second.compoundCount == 0);

    const DebugTracker &tracker = found->second;
    assert(tracker.sizeInBtye == (sizeof(float) * totalSize));
    memcpy(tracker.mappedData, points, tracker.sizeInBtye);
    return handle;

  } else {
    const DebugDrawHandle outHandle =
        drawLinesUniformColor(points, totalSize * sizeof(float), color,
                              static_cast<float>(totalSize), debugName);

    return outHandle;
  }
  */
  return {};
}

DebugDrawHandle Dx12DebugRenderer::drawAnimatedBoundingBoxFromFullPoints(
    const DebugDrawHandle handle, const glm::vec3 *data, const int count,
    const glm::vec4 color, const char *debugName) {
  // first get AABB data
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4 vertical
  // two is because we need two points per line, we are not doing trianglestrip
  const int totalSize = 4 * count * 12 * 2;  // here 4 is the xmfloat3

  auto *points = reinterpret_cast<float *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec4) * count * 12 * 2));
  int counter = 0;

  // draw vertical lines
  counter = push4toVec(points, data[0], counter);
  counter = push4toVec(points, data[2], counter);
  counter = push4toVec(points, data[0], counter);
  counter = push4toVec(points, data[3], counter);
  counter = push4toVec(points, data[0], counter);
  counter = push4toVec(points, data[4], counter);
  counter = push4toVec(points, data[2], counter);
  counter = push4toVec(points, data[6], counter);
  counter = push4toVec(points, data[4], counter);
  counter = push4toVec(points, data[6], counter);
  counter = push4toVec(points, data[2], counter);
  counter = push4toVec(points, data[5], counter);
  counter = push4toVec(points, data[3], counter);
  counter = push4toVec(points, data[7], counter);
  counter = push4toVec(points, data[3], counter);
  counter = push4toVec(points, data[5], counter);
  counter = push4toVec(points, data[1], counter);
  counter = push4toVec(points, data[6], counter);
  counter = push4toVec(points, data[1], counter);
  counter = push4toVec(points, data[5], counter);
  counter = push4toVec(points, data[1], counter);
  counter = push4toVec(points, data[7], counter);
  counter = push4toVec(points, data[4], counter);
  counter = push4toVec(points, data[7], counter);

  assert(counter <= totalSize);

  if (handle.isHandleValid()) {
    // should not be a compound
    assert((handle.handle & (1 << 31)) == 0);
    // lets get the trackers out for each one
    DebugTracker tracker{};
    const bool found = m_trackers.get(handle.handle, tracker);
    assert(found);
    assert(tracker.m_compoundCount == 1);

    const DebugDrawHandle dHandle = tracker.m_compoundHandles[0];
    assert(dHandle.isHandleValid());
    assertPoolMagicNumber(dHandle);
    uint32_t index = getIndexFromHandle(dHandle);
    const Dx12DebugPrimitive &prim = m_primitivesPool.getConstRef(index);
    // assert(prim.sizeInBtye == (sizeof(float) * totalSize));

    void *mappedData =
        globals::BUFFER_MANAGER->getMappedData(prim.m_bufferHandle);
    memcpy(mappedData, points, prim.m_sizeInByte);
    return handle;
  }
  const DebugDrawHandle linesHandle =
      drawLinesUniformColor(points, totalSize * sizeof(float), color,
                            static_cast<float>(totalSize), debugName);
  // lets prepare the compound handle
  // there are two items only lines and points and the points is the first
  DebugTracker tracker{};
  tracker.m_compoundCount = 1;
  // TODO
  // if compound is 1 or even 2.... should I just reuse the pointer memory? is a
  // bit wasteful to realloc just one handle
  tracker.m_compoundHandles = reinterpret_cast<DebugDrawHandle *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(DebugDrawHandle)));
  tracker.m_compoundHandles[0] = linesHandle;

  const DebugDrawHandle returnHandle{(MAGIC_NUMBER_COUNTER << 16) | 0};
  ++MAGIC_NUMBER_COUNTER;

  m_trackers.insert(returnHandle.handle, tracker);

  return returnHandle;
}

DebugDrawHandle Dx12DebugRenderer::drawMatrix(const glm::mat4 &mat, float size,
                                              glm::vec4 color,
                                              const char *debugName) {
  const int totalSize =
      4 * 2 * 3;  // 3 axis, each with two points, 4 floats each point
  auto *points = reinterpret_cast<float *>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(float) * totalSize));

  int counter = 0;
  // start with z axis
  glm::vec4 scaledZ = mat[2] * (size * 2.5f);
  glm::vec4 movedPosZ = mat[3] + scaledZ;
  counter = push4toVec(points, mat[3], counter);
  counter = push4toVec(points, movedPosZ, counter);

  glm::vec4 scaledX = mat[0] * size;
  glm::vec4 movedPosX = mat[3] + scaledX;
  counter = push4toVec(points, mat[3], counter);
  counter = push4toVec(points, movedPosX, counter);

  glm::vec4 scaledY = (mat[1] * (size * 1.5f));
  glm::vec4 movedPosY = mat[3] + scaledY;
  counter = push4toVec(points, mat[3], counter);
  counter = push4toVec(points, movedPosY, counter);

  DebugDrawHandle linesHandle = drawLinesUniformColor(
      points, totalSize * sizeof(float), color, totalSize, debugName);

  // lets prepare the compound handle
  // there are two items only lines and points and the points is the first
  DebugTracker tracker{};
  tracker.m_compoundCount = 1;
  // TODO
  // if compound is 1 or even 2.... should I just reuse the pointer memory? is a
  // bit wasteful to realloc just one handle
  tracker.m_compoundHandles = reinterpret_cast<DebugDrawHandle *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(DebugDrawHandle)));
  tracker.m_compoundHandles[0] = linesHandle;

  const DebugDrawHandle returnHandle{(MAGIC_NUMBER_COUNTER << 16) | 0};
  ++MAGIC_NUMBER_COUNTER;

  m_trackers.insert(returnHandle.handle, tracker);

  return returnHandle;
}
}  // namespace SirEngine::dx12