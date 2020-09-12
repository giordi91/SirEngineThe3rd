#include "platform/windows/graphics/vk/vkMaterialManager.h"

#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/skinClusterManager.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkBindingTableManager.h"
#include "platform/windows/graphics/vk/vkBufferManager.h"
#include "platform/windows/graphics/vk/vkConstantBufferManager.h"
#include "platform/windows/graphics/vk/vkMeshManager.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "platform/windows/graphics/vk/vkTextureManager.h"

namespace SirEngine::vk {

void VkMaterialManager::bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                                     const VkMaterialRuntime &materialRuntime,
                                     const VkCommandBuffer commandList) {
  int queueFlagInt = static_cast<int>(queueFlag);
  int currentFlagId = static_cast<int>(log2(queueFlagInt & -queueFlagInt));

  auto handle = materialRuntime.bindingHandle[currentFlagId];
  auto rs = materialRuntime.shaderQueueTypeFlags2[currentFlagId].rs;
  globals::BINDING_TABLE_MANAGER->bindTable(3, handle, rs, false);
}
void VkMaterialManager::bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                                     const MaterialHandle handle,
                                     VkCommandBuffer commandList) {
  const VkMaterialRuntime &materialRuntime = getMaterialRuntime(handle);
  bindMaterial(queueFlag, materialRuntime, commandList);
}

void beginRenderPass(ShaderBind bind) {
  static VkClearColorValue color{0.4, 0.4, 0.4, 1};
  // lets us start a render pass

  VkClearValue clear{};
  clear.color = color;

  VkRenderPassBeginInfo beginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  auto pass = vk::PSO_MANAGER->getRenderPassFromHandle(bind.pso);
  beginInfo.renderPass = pass;
  // beginInfo.framebuffer = m_tempFrameBuffer;

  // similar to a viewport mostly used on "tiled renderers" to optimize, talking
  // about hardware based tile renderer, aka mobile GPUs.
  beginInfo.renderArea.extent.width =
      static_cast<int32_t>(globals::ENGINE_CONFIG->m_windowWidth);
  beginInfo.renderArea.extent.height =
      static_cast<int32_t>(globals::ENGINE_CONFIG->m_windowHeight);
  beginInfo.clearValueCount = 1;
  beginInfo.pClearValues = &clear;

  vkCmdBeginRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, &beginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
}
ShaderBind VkMaterialManager::bindRSandPSO(
    const uint64_t shaderFlags, const VkMaterialRuntime &runtime,
    const VkCommandBuffer commandList) const {
  // get type flags as int
  constexpr auto mask = static_cast<uint64_t>(~((1ull << 32ull) - 1ull));
  const auto typeFlags =
      static_cast<uint64_t>((uint64_t(shaderFlags) & mask) >> 32ll);

  for (int i = 0; i < QUEUE_COUNT; ++i) {
    if (runtime.shaderQueueTypeFlags2[i].pso.handle == typeFlags) {
      ShaderBind bind = runtime.shaderQueueTypeFlags2[i];
      vk::PSO_MANAGER->bindPSO(bind.pso, commandList);
      return bind;
    }
  }
  assert(0 && "Could not find requested shader type for PSO /RS bind");
  return {};
}

inline VkDescriptorImageInfo getDescriptor(const TextureHandle handle) {
  return handle.isHandleValid() ? vk::TEXTURE_MANAGER->getSrvDescriptor(handle)
                                : VkDescriptorImageInfo{};
}

void VkMaterialManager::parseQueue(uint32_t *queues) {}

uint32_t findBindingIndex(const MaterialMetadata *meta,
                          const std::string &bindName) {
  uint32_t count = meta->objectResourceCount;
  for (int i = 0; i < count; ++i) {
    const auto &resource = meta->objectResources[i];
    bool result = strcmp(resource.name, bindName.c_str()) == 0;
    if (result) {
      return i;
    }
  }
  assert(0 && "could not find binding name");
  return 9999;
}

MaterialHandle VkMaterialManager::loadMaterial(const char *path,
                                               const MeshHandle meshHandle,
                                               const SkinHandle skinHandle) {
  PreliminaryMaterialParse parse = parseMaterial(path, meshHandle, skinHandle);

  uint32_t index;
  VkMaterialData &materialData =
      m_materialTextureHandles.getFreeMemoryData(index);

  materialData.m_material = parse.mat;
  materialData.handles = parse.handles;

  const MaterialDataHandles &handles = materialData.handles;

  VkMaterialRuntime matCpu{};
  matCpu.skinHandle = skinHandle;
  matCpu.meshHandle = meshHandle;

  for (uint32_t i = 0; i < QUEUE_COUNT; ++i) {
    const char *value = parse.shaderQueueTypeFlagsStr[i];
    if (value != nullptr) {
      PSOHandle pso = globals::PSO_MANAGER->getHandleFromName(value);
      RSHandle rs = globals::PSO_MANAGER->getRS(pso);
      matCpu.shaderQueueTypeFlags2[i] = ShaderBind{rs, pso};
    }
  }

  materialData.handles.cbHandle = globals::CONSTANT_BUFFER_MANAGER->allocate(
      sizeof(Material), 0, &parse.mat);

  materialData.magicNumber = MAGIC_NUMBER_COUNTER++;

  const std::string name = getFileName(path);
  MaterialHandle handle{(materialData.magicNumber << 16) | (index)};
  m_nameToHandle.insert(name.c_str(), handle);

  // parse material resources

  auto jobj = getJsonObj(path);
  assert(jobj.find("resources") != jobj.end());
  const auto &bindingResources = jobj["resources"];

  // NEW code avoiding the old crap
  for (uint32_t i = 0; i < QUEUE_COUNT; ++i) {
    if (!matCpu.shaderQueueTypeFlags2[i].pso.isHandleValid()) {
      continue;
    }
    ShaderBind bind = matCpu.shaderQueueTypeFlags2[i];

    uint32_t flags =
        parse.isStatic
            ? 0
            : graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED;

    const MaterialMetadata *meta = vk::PSO_MANAGER->getMetadata(bind.pso);

    uint32_t objectsCount = meta->objectResourceCount;
    // zeroing out memory jsut to be safe
    memset(descriptions, 0,
           sizeof(graphics::BindingDescription) * objectsCount);
    for (uint32_t obj = 0; obj < objectsCount; ++obj) {
      const MaterialResource &res = meta->objectResources[obj];
      auto type = res.type;
      GRAPHIC_RESOURCE_TYPE graphicsType;
      switch (type) {
        case MATERIAL_RESOURCE_TYPE::TEXTURE: {
          graphicsType = GRAPHIC_RESOURCE_TYPE::TEXTURE;
          break;
        }
        case MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER: {
          graphicsType = GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER;
          break;
        }
        case MATERIAL_RESOURCE_TYPE::BUFFER: {
          bool readOnly =
              (static_cast<uint32_t>(res.flags) &
               static_cast<uint32_t>(MATERIAL_RESOURCE_FLAGS::READ_ONLY)) > 0;
          if (readOnly) {
            graphicsType = GRAPHIC_RESOURCE_TYPE::READ_BUFFER;
          } else {
            graphicsType = GRAPHIC_RESOURCE_TYPE::READWRITE_BUFFER;
          }
          break;
        }
      }
      descriptions[obj] = {res.binding, graphicsType, res.visibility};
    }

    std::string bindingName = name + "-bindingTable";
    BindingTableHandle bindingTable =
        globals::BINDING_TABLE_MANAGER->allocateBindingTable(
            descriptions, objectsCount,
            parse.isStatic
                ? graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_NONE
                : graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
            bindingName.c_str());
    matCpu.bindingHandle[i] = bindingTable;

    // update material
    const std::string vertices = "vertices";
    const std::string normals = "normals";
    const std::string uvs = "uvs";
    const std::string tangents = "tangents";

    for (const auto &subRes : bindingResources) {
      const auto type = subRes["type"].get<std::string>();
      const auto bName = subRes["bindingName"].get<std::string>();
      const auto resPath = subRes["resourcePath"].get<std::string>();

      const std::string resName = getFileName(resPath);
      if (type == "texture") {
        TextureHandle tHandle =
            globals::TEXTURE_MANAGER->getHandleFromName(resName.c_str());

        uint32_t bindingIdx = findBindingIndex(meta, bName);

        globals::BINDING_TABLE_MANAGER->bindTexture(
            matCpu.bindingHandle[i], tHandle, bindingIdx, bindingIdx, false);
      } else if (type == "mesh") {
        MeshHandle mHandle =
            globals::MESH_MANAGER->getHandleFromName(resPath.c_str());
        auto meshFlags = static_cast<MESH_ATTRIBUTE_FLAGS>(POSITIONS | NORMALS |
                                                           UV | TANGENTS);

        globals::BINDING_TABLE_MANAGER->bindMesh(matCpu.bindingHandle[i],
                                                 mHandle, meshFlags);
      }
    }
  }
  materialData.m_materialRuntime = matCpu;

  return handle;
}

inline void freeTextureIfNeeded(const TextureHandle handle) {
  if (handle.isHandleValid()) {
    vk::TEXTURE_MANAGER->free(handle);
  }
}

void VkMaterialManager::releaseAllMaterialsAndRelatedResources() {
  int count = m_nameToHandle.binCount();
  for (int i = 0; i < count; ++i) {
    if (m_nameToHandle.isBinUsed(i)) {
      MaterialHandle value = m_nameToHandle.getValueAtBin(i);

      // now that we have the handle we can get the data
      assertMagicNumber(value);
      const uint32_t index = getIndexFromHandle(value);
      const VkMaterialData &data = m_materialTextureHandles.getConstRef(index);
      freeTextureIfNeeded(data.handles.albedo);
      freeTextureIfNeeded(data.handles.normal);
      freeTextureIfNeeded(data.handles.metallic);
      freeTextureIfNeeded(data.handles.roughness);
      freeTextureIfNeeded(data.handles.thickness);
      freeTextureIfNeeded(data.handles.separateAlpha);
      freeTextureIfNeeded(data.handles.ao);
      freeTextureIfNeeded(data.handles.height);

      // NOTE constant buffers don't need to be free singularly since the
      // rendering context will allocate in bulk

      if (data.handles.skinHandle.isHandleValid()) {
        // do not free this yet, need to figure out how
        assert(0);
        // globals::SKIN_MANAGER->free(data.handles.skinHandle);
      }

      if (data.m_materialRuntime.meshHandle.isHandleValid()) {
        vk::MESH_MANAGER->free(data.m_materialRuntime.meshHandle);
      }
    }
  }
}

void VkMaterialManager::bindTexture(const MaterialHandle matHandle,
                                    const TextureHandle texHandle,
                                    const uint32_t descriptorIndex,
                                    const uint32_t bindingIndex,
                                    SHADER_QUEUE_FLAGS queue,
                                    const bool isCubeMap) {
  assertMagicNumber(matHandle);
  uint32_t index = getIndexFromHandle(matHandle);
  const auto &data = m_materialTextureHandles.getConstRef(index);

  const auto currentFlag = static_cast<uint32_t>(queue);
  int currentFlagId = static_cast<int>(log2(currentFlag & -currentFlag));

  // this is the descriptor for our correct queue
  DescriptorHandle descriptorHandle =
      data.m_materialRuntime.descriptorHandles[currentFlagId];
  assert(descriptorHandle.isHandleValid());
  // the descriptor set is already taking into account whether or not
  // is buffered, it gives us the correct one we want
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(descriptorHandle);

  // assert(!vk::DESCRIPTOR_MANAGER->isBuffered(descriptorHandle) &&
  //       "buffered not yet implemented");

  VkWriteDescriptorSet writeDescriptorSets{};

  vk::TEXTURE_MANAGER->bindTexture(texHandle, &writeDescriptorSets,
                                   descriptorSet, bindingIndex);

  // Execute the writes to update descriptors for this set
  // Note that it's also possible to gather all writes and only run updates
  // once, even for multiple sets This is possible because each
  // VkWriteDescriptorSet also contains the destination set to be updated
  // For simplicity we will update once per set instead
  // object one off update
  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 1, &writeDescriptorSets, 0,
                         nullptr);
}

void VkMaterialManager::bindBuffer(MaterialHandle matHandle,
                                   BufferHandle bufferHandle,
                                   uint32_t bindingIndex,
                                   SHADER_QUEUE_FLAGS queue) {
  assertMagicNumber(matHandle);
  uint32_t index = getIndexFromHandle(matHandle);
  const auto &data = m_materialTextureHandles.getConstRef(index);

  const uint32_t currentFlag = static_cast<uint32_t>(queue);
  int currentFlagId = static_cast<int>(log2(currentFlag & -currentFlag));

  DescriptorHandle descriptorHandle =
      data.m_materialRuntime.descriptorHandles[currentFlagId];
  assert(descriptorHandle.isHandleValid());
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(descriptorHandle);

  VkWriteDescriptorSet writeDescriptorSets{};

  vk::BUFFER_MANAGER->bindBuffer(bufferHandle, &writeDescriptorSets,
                                 descriptorSet, bindingIndex);

  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 1, &writeDescriptorSets, 0,
                         nullptr);
}

void VkMaterialManager::bindMaterial(const MaterialHandle handle,
                                     SHADER_QUEUE_FLAGS queue) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const auto &data = m_materialTextureHandles.getConstRef(index);

  const auto currentFlag = static_cast<uint32_t>(queue);
  int currentFlagId = static_cast<int>(log2(currentFlag & -currentFlag));

  //// find the PSO, should this be part of the runtime directly?
  // uint32_t flags =
  // data.m_materialRuntime.shaderQueueTypeFlags[currentFlagId];
  // SHADER_TYPE_FLAGS type = getTypeFlags(flags);
  ShaderBind bind = data.m_materialRuntime.shaderQueueTypeFlags2[currentFlagId];
  // bool found = m_shaderTypeToShaderBind.get(static_cast<uint16_t>(type),
  // bind); assert(found);
  /*

  vk::PSO_MANAGER->bindPSO(bind.pso, CURRENT_FRAME_COMMAND->m_commandBuffer);

  DescriptorHandle descriptorHandle =
      data.m_materialRuntime.descriptorHandles[currentFlagId];
  assert(descriptorHandle.isHandleValid());
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(descriptorHandle);

  VkPipelineLayout layout = data.m_materialRuntime.layouts[currentFlagId];
  assert(layout != nullptr);

  VkDescriptorSet sets[] = {descriptorSet};
  // multiple descriptor sets
  vkCmdBindDescriptorSets(
      CURRENT_FRAME_COMMAND->m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      layout, PSOManager::PER_OBJECT_BINDING_INDEX, 1, sets, 0, nullptr);
  */

  globals::BINDING_TABLE_MANAGER->bindTable(
      3, data.m_materialRuntime.bindingHandle[currentFlagId], bind.rs, false);
}

void VkMaterialManager::free(const MaterialHandle handle) {
  // TODO properly cleanup the resources
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const auto &data = m_materialTextureHandles.getConstRef(index);

  if (data.name != nullptr) {
    m_nameToHandle.remove(data.name);
  }

  m_materialTextureHandles.free(index);
}

void VkMaterialManager::bindMesh(const MaterialHandle handle,
                                 const MeshHandle meshHandle,
                                 const uint32_t descriptorIndex,
                                 const uint32_t bindingIndex,
                                 const uint32_t meshBindFlags,
                                 SHADER_QUEUE_FLAGS queue) {
  const auto &materialRuntime = getMaterialRuntime(handle);
  int queueFlagInt = static_cast<int>(queue);
  int currentFlagId = static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  DescriptorHandle setHandle = materialRuntime.descriptorHandles[currentFlagId];
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(setHandle);

  // TODO here we are assuming always four sets might have to rework this
  VkWriteDescriptorSet writeDescriptorSets[4] = {};

  VkDescriptorBufferInfo bufferInfo[3] = {};
  vk::MESH_MANAGER->bindMesh(meshHandle, &writeDescriptorSets[bindingIndex],
                             descriptorSet, bufferInfo, meshBindFlags,
                             bindingIndex);

  int setPos = (meshBindFlags & MESH_ATTRIBUTE_FLAGS::POSITIONS) > 0 ? 1 : 0;
  int setNormals = (meshBindFlags & MESH_ATTRIBUTE_FLAGS::NORMALS) > 0 ? 1 : 0;
  int setUV = (meshBindFlags & MESH_ATTRIBUTE_FLAGS::UV) > 0 ? 1 : 0;
  int setTangents =
      (meshBindFlags & MESH_ATTRIBUTE_FLAGS::TANGENTS) > 0 ? 1 : 0;

  int toBind = setPos + setNormals + setUV + setTangents;

  // Execute the writes to update descriptors for this set
  // Note that it's also possible to gather all writes and only run updates
  // once, even for multiple sets This is possible because each
  // VkWriteDescriptorSet also contains the destination set to be updated
  // For simplicity we will update once per set instead
  // object one off update
  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, toBind, &writeDescriptorSets[0], 0,
                         nullptr);
}

void VkMaterialManager::bindConstantBuffer(
    const MaterialHandle handle, const ConstantBufferHandle bufferHandle,
    const uint32_t descriptorIndex, const uint32_t bindingIndex,
    SHADER_QUEUE_FLAGS queue) {
  const auto &materialRuntime = getMaterialRuntime(handle);
  int queueFlagInt = static_cast<int>(queue);
  int currentFlagId = static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  DescriptorHandle setHandle = materialRuntime.descriptorHandles[currentFlagId];
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(setHandle);

  VkWriteDescriptorSet writeDescriptorSets = {};
  VkDescriptorBufferInfo bufferInfoUniform = {};
  vk::CONSTANT_BUFFER_MANAGER->bindConstantBuffer(
      bufferHandle, bufferInfoUniform, bindingIndex, &writeDescriptorSets,
      descriptorSet);

  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 1, &writeDescriptorSets, 0,
                         nullptr);
}
}  // namespace SirEngine::vk
