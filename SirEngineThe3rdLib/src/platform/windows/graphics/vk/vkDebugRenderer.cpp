#include "platform/windows/graphics/vk/vkDebugRenderer.h"

#include "SirEngine/globals.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/vk/vkBufferManager.h"
#include "platform/windows/graphics/vk/vkConstantBufferManager.h"
#include "platform/windows/graphics/vk/vkBindingTableManager.h"
#include "platform/windows/graphics/vk/vkMaterialManager.h"
#include "vk.h"

namespace SirEngine::vk {

void VkDebugRenderer::initialize() {}
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

VkDebugRenderer::VkDebugRenderer()
    : DebugRenderer(),
      m_primitivesPool(RESERVE_SIZE),
      m_trackers(RESERVE_SIZE) {}

void VkDebugRenderer::cleanup() {}

void VkDebugRenderer::free(const DebugDrawHandle handle) {
  assertMagicNumber(handle);

  DebugTracker tracker{};
  const bool found = m_trackers.get(handle.handle, tracker);
  assert(found);
  const int primCount = tracker.compoundCount;
  for (int i = 0; i < primCount; ++i) {
    const DebugDrawHandle compHandle = tracker.compoundHandles[i];
    assertPoolMagicNumber(compHandle);
    const uint32_t index = getIndexFromHandle(handle);
    const VkDebugPrimitive &prim = m_primitivesPool.getConstRef(index);
    if (prim.m_bufferHandle.isHandleValid()) {
      globals::BUFFER_MANAGER->free(prim.m_bufferHandle);
    }
    if (prim.m_cbHandle.isHandleValid()) {
      globals::CONSTANT_BUFFER_MANAGER->free(prim.m_cbHandle);
    }
    m_primitivesPool.free(index);
  }
  m_trackers.remove(handle.handle);
}

DebugDrawHandle VkDebugRenderer::drawPointsUniformColor(float *data,
                                                        uint32_t sizeInByte,
                                                        glm::vec4 color,
                                                        float size,
                                                        const char *debugName) {
  assert(0);
  return {};
}

DebugDrawHandle VkDebugRenderer::drawLinesUniformColor(float *data,
                                                       uint32_t sizeInByte,
                                                       glm::vec4 color,
                                                       float size,
                                                       const char *debugName) {
  uint32_t index;
  VkDebugPrimitive &primitive = m_primitivesPool.getFreeMemoryData(index);

  // allocate vertex buffer
  assert((sizeInByte % (sizeof(float) * 3)) == 0);
  const uint32_t elementCount = sizeInByte / (sizeof(float) * 3);

  const BufferHandle bufferHandle = globals::BUFFER_MANAGER->allocate(
      sizeInByte, data, debugName, elementCount, sizeof(float) * 3,
      BufferManager::BUFFER_FLAGS::STORAGE_BUFFER);
  primitive.m_bufferHandle = bufferHandle;

  // allocate constant buffer
  DebugPointsFixedColor settings{color, size, {0, 0, 0}};
  const ConstantBufferHandle chandle =
      globals::CONSTANT_BUFFER_MANAGER->allocate(sizeof(settings), 0,
                                                 &settings);

  RenderableDescription description{};
  description.buffer = bufferHandle;
  description.subranges[0].m_offset = 0;
  description.subranges[0].m_size = sizeInByte;
  description.subragesCount = 1;

  const char *queues[5] = {nullptr, nullptr, nullptr, "debugLinesSingleColor",
                           nullptr};

  description.materialHandle = globals::MATERIAL_MANAGER->allocateMaterial(
      debugName, MaterialManager::ALLOCATE_MATERIAL_FLAG_BITS::NONE, queues);
  description.primitiveToRender = elementCount;

  const VkMaterialRuntime &runtime =
      vk::MATERIAL_MANAGER->getMaterialRuntime(description.materialHandle);

  // TODO do we need that? we should be able to query from the material
  // manager for the wanted queue
  const auto currentFlag = static_cast<uint32_t>(SHADER_QUEUE_FLAGS::DEBUG);
  const int currentFlagId =
      static_cast<int>(log2(currentFlag & (-currentFlag)));

  VkDescriptorSet set = vk::DESCRIPTOR_MANAGER->getDescriptorSet(
      runtime.descriptorHandles[currentFlagId]);

  VkWriteDescriptorSet writeDescriptorSet{};
  VkDescriptorBufferInfo info{};

  // TODO here two different updates descriptor updates are performed
  // slower but "cleaner", need to figure out both a nice and fast way
  // TODO here we can see the difference between a "generic" material set
  // and the Vulkan aware one, where we can pass in vk structure to be filled
  // than have one write only
  globals::MATERIAL_MANAGER->bindBuffer(description.materialHandle,
                                        primitive.m_bufferHandle, 0,
                                        SHADER_QUEUE_FLAGS::DEBUG);

  vk::CONSTANT_BUFFER_MANAGER->bindConstantBuffer(chandle, info, 1,
                                                  &writeDescriptorSet, set);

  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 1, &writeDescriptorSet, 0,
                         nullptr);

  globals::RENDERING_CONTEXT->addRenderablesToQueue(description);

  // store it such way that we can render it
  primitive.m_primitiveType = PRIMITIVE_TYPE::LINE;
  primitive.m_cbHandle = chandle;
  primitive.m_primitiveToRender = static_cast<int>(elementCount);
  primitive.m_magicNumber = MAGIC_NUMBER_COUNTER;

  const DebugDrawHandle debugHandle{(MAGIC_NUMBER_COUNTER << 16) | index};
  ++MAGIC_NUMBER_COUNTER;

  DebugTracker tracker{};
  tracker.magicNumber = MAGIC_NUMBER_COUNTER;
  tracker.mappedData = nullptr;
  // only one object, this should be renamed to normal counter not compound
  // simply set to one if not compound
  tracker.compoundCount = 1;
  tracker.compoundHandles = reinterpret_cast<DebugDrawHandle *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(DebugDrawHandle) * 1));
  tracker.compoundHandles[0] = debugHandle;
  tracker.sizeInBtye = sizeInByte;

  const DebugDrawHandle trackerHandle{(MAGIC_NUMBER_COUNTER << 16)};

  // registering the tracker
  m_trackers.insert(trackerHandle.handle, tracker);

  ++MAGIC_NUMBER_COUNTER;

  return trackerHandle;
}

DebugDrawHandle VkDebugRenderer::drawSkeleton(Skeleton *skeleton,
                                              glm::vec4 color,
                                              float pointSize) {
  assert(0);
  return {};
}

DebugDrawHandle VkDebugRenderer::drawAnimatedSkeleton(DebugDrawHandle handle,
                                                      AnimationPlayer *state,
                                                      glm::vec4 color,
                                                      float pointSize) {
  assert(0);
  return {};
}

void VkDebugRenderer::render(TextureHandle input, TextureHandle depth) {
  auto commandList = &vk::CURRENT_FRAME_COMMAND->m_commandBuffer;

  const DrawCallConfig config{
      static_cast<uint32_t>(globals::ENGINE_CONFIG->m_windowWidth),
      static_cast<uint32_t>(globals::ENGINE_CONFIG->m_windowHeight), 0};
  globals::RENDERING_CONTEXT->renderQueueType(config,
                                              SHADER_QUEUE_FLAGS::DEBUG);
}

DebugDrawHandle VkDebugRenderer::drawBoundingBoxes(const BoundingBox *data,
                                                   const int count,
                                                   const glm::vec4 color,
                                                   const char *debugName) {
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4
  // vertical two is because we need two points per line, we are not doing
  // triangle-strip
  const int totalSize = 4 * count * 12 * 2;  // here 4 is the xmfloat4

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
  return drawLinesUniformColor(points, totalSize * sizeof(float), color,
                               static_cast<float>(totalSize), debugName);
}

DebugDrawHandle VkDebugRenderer::drawAnimatedBoundingBoxes(
    DebugDrawHandle handle, BoundingBox *data, int count, glm::vec4 color,
    const char *debugName) {
  assert(0);
  return {};
}

DebugDrawHandle VkDebugRenderer::drawAnimatedBoundingBoxFromFullPoints(
    const DebugDrawHandle handle, const glm::vec3 *data, const int count,
    const glm::vec4 color, const char *debugName) {
  assert(0);
  return {};
}

DebugDrawHandle VkDebugRenderer::drawMatrix(const glm::mat4 &mat, float size,
                                            glm::vec4 color,
                                            const char *debugName) {
  assert(0);
  return {};
}
}  // namespace SirEngine::vk
