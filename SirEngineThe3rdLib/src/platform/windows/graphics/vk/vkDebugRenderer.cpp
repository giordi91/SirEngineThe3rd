#include "platform/windows/graphics/vk/vkDebugRenderer.h"

#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "platform/windows/graphics/vk/vkRootSignatureManager.h"
#include "vkBufferManager.h"
#include "vkConstantBufferManager.h"
#include "vkDescriptorManager.h"
#include "vkMaterialManager.h"

namespace SirEngine::vk {
void VkDebugRenderer::initialize() {
  // lets build the map of rs and pso
  // points single color
  PSOHandle psoHandle =
      vk::PSO_MANAGER->getHandleFromName("debugDrawPointsSingleColorPSO");
  RSHandle rsHandle = vk::PIPELINE_LAYOUT_MANAGER->getHandleFromName(
      "debugDrawPointsSingleColorRS");

  m_shderTypeToShaderBind.insert(
      static_cast<uint16_t>(SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR),
      ShaderBind{rsHandle, psoHandle});

  psoHandle =
      vk::PSO_MANAGER->getHandleFromName("debugDrawLinesSingleColorPSO");
  rsHandle = vk::PIPELINE_LAYOUT_MANAGER->getHandleFromName(
      "debugDrawLinesSingleColorRS");

  m_shderTypeToShaderBind.insert(
      static_cast<uint16_t>(SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR),
      ShaderBind{rsHandle, psoHandle});
}
inline int push3toVec(float* data, const glm::vec4 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;

  return counter;
}
inline int push3toVec(float* data, const glm::vec3 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;

  return counter;
}
inline int push4toVec(float* data, const glm::vec3 v, int counter) {
  data[counter++] = v.x;
  data[counter++] = v.y;
  data[counter++] = v.z;
  data[counter++] = 1.0f;

  return counter;
}
inline int push3toVec(float* data, float x, float y, float z, int counter) {
  data[counter++] = x;
  data[counter++] = y;
  data[counter++] = z;

  return counter;
}
inline int push4toVec(float* data, float x, float y, float z, int counter) {
  data[counter++] = x;
  data[counter++] = y;
  data[counter++] = z;
  data[counter++] = 1.0f;

  return counter;
}
int drawSquareBetweenTwoPoints(float* data, const glm::vec3 minP,
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

void VkDebugRenderer::cleanup() {}

DebugDrawHandle VkDebugRenderer::drawPointsUniformColor(float* data,
                                                        uint32_t sizeInByte,
                                                        glm::vec4 color,
                                                        float size,
                                                        const char* debugName) {
  assert(0);
  return {};
}

DebugDrawHandle VkDebugRenderer::drawLinesUniformColor(float* data,
                                                       uint32_t sizeInByte,
                                                       glm::vec4 color,
                                                       float size,
                                                       const char* debugName) {
  VkDebugPrimitive primitive{};

  // allocate vertex buffer
  assert((sizeInByte % (sizeof(float) * 3)) == 0);
  const uint32_t elementCount = sizeInByte / (sizeof(float) * 3);

  BufferHandle bufferHandle = vk::BUFFER_MANAGER->allocate(
      sizeInByte, data, debugName, elementCount, sizeof(float) * 3,
      BufferManager::BUFFER_FLAGS::STORAGE_BUFFER);
  primitive.bufferHandle = bufferHandle;

  // allocate constant buffer
  DebugPointsFixedColor settings{color, size};
  const ConstantBufferHandle chandle =
      vk::CONSTANT_BUFFER_MANAGER->allocate(sizeof(settings), 0, &settings);

  ShaderBind bind{};
  bool found = m_shderTypeToShaderBind.get(
      static_cast<uint16_t>(SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR), bind);

  const char* descriptorName = frameConcatenation(debugName, "Data");

  //// not buffered
  // uint32_t flags = 0;
  // primitive.descriptorHandle =
  //    vk::DESCRIPTOR_MANAGER->allocate(bind.rs, flags, descriptorName);

  // primitive.layout =
  // vk::PIPELINE_LAYOUT_MANAGER->getLayoutFromHandle(bind.rs);

  // VkWriteDescriptorSet writeDescriptorSet[2]{};
  // VkDescriptorBufferInfo info[2]{};
  // VkDescriptorSet descriptorSet =
  //    vk::DESCRIPTOR_MANAGER->getDescriptorSet(primitive.descriptorHandle);
  //// updating the descriptors
  // info[0].buffer =
  // vk::BUFFER_MANAGER->getNativeBuffer(primitive.bufferHandle); info[0].offset
  // = 0; info[0].range = sizeInByte;

  // writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  // writeDescriptorSet[0].dstSet = descriptorSet;
  // writeDescriptorSet[0].dstBinding = 0;
  // writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  // writeDescriptorSet[0].pBufferInfo = &info[0];
  // writeDescriptorSet[0].descriptorCount = 1;

  // vk::CONSTANT_BUFFER_MANAGER->bindConstantBuffer(
  //    chandle, info[1], 1, writeDescriptorSet, descriptorSet);

  // vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 2, writeDescriptorSet, 0,
  // nullptr);

  RenderableDescription description{};
  description.buffer = bufferHandle;
  description.subranges[0].m_offset = 0;
  description.subranges[0].m_size = sizeInByte;
  description.subragesCount = 1;
  description.materialHandle = globals::MATERIAL_MANAGER->allocateMaterial(
      "debugLinesSingleColor", debugName,
      MaterialManager::ALLOCATE_MATERIAL_FLAG_BITS::NONE);
  description.primitiveToRender = elementCount;

  VkMaterialData& r =
      const_cast<VkMaterialData&>(vk::MATERIAL_MANAGER->getMaterialData(description.materialHandle));

  // setting the correct queue
  VkMaterialRuntime& runtime = r.m_materialRuntime;
  for (int i = 0; i < 4; ++i) {
    runtime.shaderQueueTypeFlags[i] = INVALID_QUEUE_TYPE_FLAGS;
  }

  uint32_t currentFlag = static_cast<uint32_t>(SHADER_QUEUE_FLAGS::DEBUG);
  int currentFlagId = static_cast<int>(log2(currentFlag & -currentFlag));

  runtime.shaderQueueTypeFlags[currentFlagId] =
      currentFlag |
      ((static_cast<uint32_t>(SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR)
        << 16));
  runtime.descriptorHandles[currentFlagId] = r.m_descriptorHandle;
  runtime.layouts[currentFlagId] =
   vk::PIPELINE_LAYOUT_MANAGER->getLayoutFromHandle(bind.rs);
  runtime.useStaticSamplers[currentFlagId] = 0;

  DescriptorHandle desc = r.m_descriptorHandle;
  VkDescriptorSet set = vk::DESCRIPTOR_MANAGER->getDescriptorSet(desc);

  VkWriteDescriptorSet writeDescriptorSet[2]{};
  VkDescriptorBufferInfo info[2]{};
  // updating the descriptors
  info[0].buffer = vk::BUFFER_MANAGER->getNativeBuffer(primitive.bufferHandle);
  info[0].offset = 0;
  info[0].range = sizeInByte;

  writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet[0].dstSet = set;
  writeDescriptorSet[0].dstBinding = 0;
  writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writeDescriptorSet[0].pBufferInfo = &info[0];
  writeDescriptorSet[0].descriptorCount = 1;

  vk::CONSTANT_BUFFER_MANAGER->bindConstantBuffer(chandle, info[1], 1,
                                                  writeDescriptorSet, set);

  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 2, writeDescriptorSet, 0, nullptr);

  globals::RENDERING_CONTEXT->addRenderablesToQueue(description);

  // store it such way that we can render it
  primitive.primitiveType = PRIMITIVE_TYPE::LINE;
  primitive.cbHandle = chandle;
  primitive.primitiveToRender = static_cast<int>(elementCount);

  // generate handle for storing
  SHADER_QUEUE_FLAGS queue = SHADER_QUEUE_FLAGS::DEBUG;
  SHADER_TYPE_FLAGS type = SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR;
  const uint32_t storeHandle =
      static_cast<uint32_t>(queue) | (static_cast<uint32_t>(type) << 16);

  DebugTracker tracker{};
  tracker.compoundCount = 0;
  tracker.index = m_renderables[storeHandle].size();
  tracker.magicNumber = MAGIC_NUMBER_COUNTER;
  tracker.queue = storeHandle;
  tracker.mappedData = nullptr;
  tracker.sizeInBtye = sizeInByte;

  const DebugDrawHandle debugHandle{(MAGIC_NUMBER_COUNTER << 16) |
                                    tracker.index};

  // registering the tracker
  m_trackers.insert(debugHandle.handle, tracker);
  // registering the renderables
  m_renderables[storeHandle].push_back(primitive);

  ++MAGIC_NUMBER_COUNTER;

  return debugHandle;
}

DebugDrawHandle VkDebugRenderer::drawSkeleton(Skeleton* skeleton,
                                              glm::vec4 color,
                                              float pointSize) {
  assert(0);
  return {};
}

DebugDrawHandle VkDebugRenderer::drawAnimatedSkeleton(DebugDrawHandle handle,
                                                      AnimationPlayer* state,
                                                      glm::vec4 color,
                                                      float pointSize) {
  assert(0);
  return {};
}
void VkDebugRenderer::renderQueue(
    std::unordered_map<uint32_t, std::vector<VkDebugPrimitive>>& inQueue,
    const TextureHandle input, const TextureHandle depth) {
  auto* currentFc = CURRENT_FRAME_COMMAND;
  auto commandList = currentFc->m_commandBuffer;


  DrawCallConfig config{
      globals::ENGINE_CONFIG->m_windowWidth,
      globals::ENGINE_CONFIG->m_windowHeight,
      static_cast<uint32_t>(DRAW_CALL_FLAGS::SHOULD_CLEAR_COLOR),
      glm::vec4(0.4f, 0.4f, 0.4f, 1.0f),
  };
  globals::RENDERING_CONTEXT->renderQueueType(config,
                                              SHADER_QUEUE_FLAGS::DEBUG);

  return;

  for (auto& queue : inQueue) {
    assert((globals::MATERIAL_MANAGER->isQueueType(queue.first,
                                                   SHADER_QUEUE_FLAGS::DEBUG)));

    // get type flags as int
    uint32_t shaderFlags = queue.first;
    constexpr auto mask = static_cast<uint32_t>(~((1 << 16) - 1));
    const auto typeFlags = static_cast<uint16_t>((shaderFlags & mask) >> 16);

    ShaderBind bind;
    bool found = m_shderTypeToShaderBind.get(typeFlags, bind);
    if (found) {
      // beginRenderPass(bind);
      vk::PSO_MANAGER->bindPSO(bind.pso, commandList);
    } else {
      assert(!"Could not find debug pso or rs");
    }

    // this is most for debug, it will boil down to nothing in release
    const SHADER_TYPE_FLAGS type =
        globals::MATERIAL_MANAGER->getTypeFlags(queue.first);

    for (auto& prim : queue.second) {
      VkDescriptorSet descriptorSet =
          vk::DESCRIPTOR_MANAGER->getDescriptorSet(prim.descriptorHandle);
      VkDescriptorSet sets[] = {
          vk::DESCRIPTOR_MANAGER->getDescriptorSet(PER_FRAME_DATA_HANDLE),
          descriptorSet, vk::STATIC_SAMPLERS_DESCRIPTOR_SET};
      // multiple descriptor sets
      vkCmdBindDescriptorSets(commandList, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              prim.layout, 0, 2, sets, 0, nullptr);

      // wheter you render triangle or lines, is embedded in the pso, no need to
      // set manually
      const uint32_t isPC = type == SHADER_TYPE_FLAGS::DEBUG_POINTS_COLORS;
      const uint32_t isPSC =
          type == SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR;

      if (isPC | isPSC) {
      } else {
      }
      vkCmdDraw(commandList, prim.primitiveToRender, 1, 0, 0);
    }
  }
}

void VkDebugRenderer::render(TextureHandle input, TextureHandle depth) {
  auto commandList = &vk::CURRENT_FRAME_COMMAND->m_commandBuffer;
  // first static stuff
  renderQueue(m_renderables, input, depth);
}

void VkDebugRenderer::clearUploadRequests() {}

DebugDrawHandle VkDebugRenderer::drawBoundingBoxes(BoundingBox* data, int count,
                                                   glm::vec4 color,
                                                   const char* debugName) {
  // 12 is the number of lines needed for the AABB, 4 top, 4 bottom, 4 vertical
  // two is because we need two points per line, we are not doing trianglestrip
  int totalSize = 4 * count * 12 * 2;  // here 4 is the xmfloat4

  auto* points = reinterpret_cast<float*>(
      globals::FRAME_ALLOCATOR->allocate(sizeof(glm::vec4) * count * 12 * 2));
  int counter = 0;
  for (int i = 0; i < count; ++i) {
    assert(counter <= totalSize);
    const auto& minP = data[i].min;
    const auto& maxP = data[i].max;
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

DebugDrawHandle VkDebugRenderer::drawAnimatedBoundingBoxes(
    DebugDrawHandle handle, BoundingBox* data, int count, glm::vec4 color,
    const char* debugName) {
  assert(0);
  return {};
}

DebugDrawHandle VkDebugRenderer::drawAnimatedBoundingBoxFromFullPoints(
    const DebugDrawHandle handle, const glm::vec3* data, const int count,
    const glm::vec4 color, const char* debugName) {
  assert(0);
  return {};
}

void VkDebugRenderer::drawMatrix(const glm::mat4& mat, float size,
                                 glm::vec4 color, const char* debugName) {
  assert(0);
}
}  // namespace SirEngine::vk
