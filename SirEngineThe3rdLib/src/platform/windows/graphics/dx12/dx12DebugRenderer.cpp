#include "platform/windows/graphics/dx12/dx12DebugRenderer.h"

#include "SirEngine/PSOManager.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/cpu/resizableVector.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"
#include "platform/windows/graphics/vk/vkBindingTableManager.h"
#include "platform/windows/graphics/vk/vkBufferManager.h"
#include "platform/windows/graphics/vk/vkConstantBufferManager.h"
#include "platform/windows/graphics/vk/vkMaterialManager.h"

namespace SirEngine::dx12 {
static const char *LINE_RS = "debugDrawLinesSingleColorRS";
static const char *LINE_PSO = "debugDrawLinesSingleColorPSO";

void Dx12DebugRenderer::initialize() {
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

Dx12DebugRenderer::Dx12DebugRenderer()
    : DebugRenderer(),
      m_primitivesPool(RESERVE_SIZE),
      m_trackers(RESERVE_SIZE),
      m_lineBindHandles(HANDLES_INITIAL_SIZE) {}

void Dx12DebugRenderer::cleanup() {
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

void Dx12DebugRenderer::free(const DebugDrawHandle handle) {
  assertMagicNumber(handle);

  DebugTracker tracker{};
  const bool found = m_trackers.get(handle.handle, tracker);
  assert(found);
  const int primCount = tracker.compoundCount;
  for (int i = 0; i < primCount; ++i) {
    const DebugDrawHandle compHandle = tracker.compoundHandles[i];
    assertPoolMagicNumber(compHandle);
    const uint32_t index = getIndexFromHandle(handle);
    const Dx12DebugPrimitive &prim = m_primitivesPool.getConstRef(index);
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

DebugDrawHandle Dx12DebugRenderer::drawPointsUniformColor(float *data,
                                                        uint32_t sizeInByte,
                                                        glm::vec4 color,
                                                        float size,
                                                        const char *debugName) {
  assert(0);
  return {};
}

DebugDrawHandle Dx12DebugRenderer::drawLinesUniformColor(float *data,
                                                       uint32_t sizeInByte,
                                                       glm::vec4 color,
                                                       float size,
                                                       const char *debugName) {
  /*
uint32_t index;
Dx12DebugPrimitive &primitive = m_primitivesPool.getFreeMemoryData(index);

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
*/
  return {};
}

DebugDrawHandle Dx12DebugRenderer::drawSkeleton(Skeleton *skeleton,
                                              glm::vec4 color,
                                              float pointSize) {
  assert(0);
  return {};
}

DebugDrawHandle Dx12DebugRenderer::drawAnimatedSkeleton(DebugDrawHandle handle,
                                                      AnimationPlayer *state,
                                                      glm::vec4 color,
                                                      float pointSize) {
  assert(0);
  return {};
}

void Dx12DebugRenderer::render(TextureHandle input, TextureHandle depth) {
  // draw lines
  int slabCount = m_lineSlab[globals::CURRENT_FRAME].getSlabCount();
  assureLinesTables(slabCount);

  for (int i = 0; i < slabCount; ++i) {

    uint32_t allocatedSize =
        m_lineSlab[globals::CURRENT_FRAME].getAllocatedBytes(i);
    assert((allocatedSize % 8 == 0));
    uint32_t primCount = allocatedSize / (sizeof(float) * 8);
    if(primCount == 0 )
    {
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

  // m_linesPrimitives =0;
}

DebugDrawHandle Dx12DebugRenderer::drawBoundingBoxes(const BoundingBox *data,
                                                   const int count,
                                                   const glm::vec4 color,
                                                   const char *debugName) {
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4
  // vertical two is because we need two points per line, we are not doing
  // triangle-strip
  const int totalSize = 4 * count * 12 * 2;  // here 4 is the xmfloat4

  auto *points = static_cast<float *>(
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

DebugDrawHandle Dx12DebugRenderer::drawAnimatedBoundingBoxes(
    DebugDrawHandle handle, BoundingBox *data, int count, glm::vec4 color,
    const char *debugName) {
  assert(0);
  return {};
}

DebugDrawHandle Dx12DebugRenderer::drawAnimatedBoundingBoxFromFullPoints(
    const DebugDrawHandle handle, const glm::vec3 *data, const int count,
    const glm::vec4 color, const char *debugName) {
  assert(0);
  return {};
}

DebugDrawHandle Dx12DebugRenderer::drawMatrix(const glm::mat4 &mat, float size,
                                            glm::vec4 color,
                                            const char *debugName) {
  assert(0);
  return {};
}

void Dx12DebugRenderer::drawLines(float *data, const uint32_t sizeInByte,
                                const glm::vec4 color, float size,
                                const char *debugName) {
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

  m_linesPrimitives += count;
  // ignoring the handle we are going to flush every frame
  m_lineSlab[globals::CURRENT_FRAME].allocate(finalSize, paddedData);
}

void Dx12DebugRenderer::updateBoundingBoxesData(const DebugDrawHandle handle,
                                              const BoundingBox *data,
                                              const int count) {
  // assertMagicNumber(handle);
  // assertPoolMagicNumber(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  Dx12DebugPrimitive &debug = m_primitivesPool[idx];
  BufferHandle bufferHandle = debug.m_bufferHandle;
  void *mappedData = globals::BUFFER_MANAGER->getMappedData(bufferHandle);

  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4
  // vertical two is because we need two points per line, we are not doing
  // triangle-strip
  const int totalSize = 4 * count * 12 * 2;  // here 4 is the xmfloat4

  auto *points = static_cast<float *>(
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

  memcpy(mappedData, points, totalSize * sizeof(float));
}

void Dx12DebugRenderer::assureLinesTables(const int slabCount) {
  // init binding table
  graphics::BindingDescription descriptions[] = {
      {0, GRAPHIC_RESOURCE_TYPE::READ_BUFFER,
       GRAPHICS_RESOURCE_VISIBILITY_VERTEX}};

  int count = m_lineBindHandles.size();
  for (int i = count; i < slabCount; ++i) {
    BindingTableHandle handle =
        globals::BINDING_TABLE_MANAGER->allocateBindingTable(
            descriptions, ARRAYSIZE(descriptions),
            graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
            "debugLines");
    m_lineBindHandles.pushBack(handle.handle);
  }
}

void Dx12DebugRenderer::newFrame() {
  m_lineSlab[globals::CURRENT_FRAME].clear();
  m_linesPrimitives = 0;
}
}  // namespace SirEngine::dx12