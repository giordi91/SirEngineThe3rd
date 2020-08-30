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
#include "platform/windows/graphics/vk/vkRootSignatureManager.h"
#include "platform/windows/graphics/vk/vkTextureManager.h"

namespace SirEngine::vk {

void updateForwardPhong(SHADER_QUEUE_FLAGS queueFlag,
                        const VkMaterialRuntime &materialRuntime) {
  // Update the descriptor set with the actual descriptors matching shader
  // bindings set in the layout
  // this far we defined just what descriptor we wanted and how they were setup,
  // now we need to actually define the content of those descriptors, the actual
  // resources

  int queueFlagInt = static_cast<int>(queueFlag);
  int currentFlagId = static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  DescriptorHandle setHandle = materialRuntime.descriptorHandles[currentFlagId];
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(setHandle);

  VkWriteDescriptorSet writeDescriptorSets[8] = {};

  uint32_t flags = POSITIONS | NORMALS | UV | TANGENTS;
  VkDescriptorBufferInfo bufferInfo[4] = {};
  vk::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, writeDescriptorSets,
                             descriptorSet, bufferInfo, flags, 0);
  // root, bufferInfo);

  vk::TEXTURE_MANAGER->bindTexture(materialRuntime.albedo,
                                   &writeDescriptorSets[4], descriptorSet, 4);
  vk::TEXTURE_MANAGER->bindTexture(materialRuntime.normal,
                                   &writeDescriptorSets[5], descriptorSet, 5);
  vk::TEXTURE_MANAGER->bindTexture(materialRuntime.metallic,
                                   &writeDescriptorSets[6], descriptorSet, 6);
  vk::TEXTURE_MANAGER->bindTexture(materialRuntime.roughness,
                                   &writeDescriptorSets[7], descriptorSet, 7);

  // Execute the writes to update descriptors for this set
  // Note that it's also possible to gather all writes and only run updates
  // once, even for multiple sets This is possible because each
  // VkWriteDescriptorSet also contains the destination set to be updated
  // For simplicity we will update once per set instead
  // object one off update
  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 8, &writeDescriptorSets[0], 0,
                         nullptr);
}

void VkMaterialManager::bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                                     const VkMaterialRuntime &materialRuntime,
                                     VkCommandBuffer commandList) {
  int queueFlagInt = static_cast<int>(queueFlag);
  int currentFlagId = static_cast<int>(log2(queueFlagInt & -queueFlagInt));

  DescriptorHandle setHandle = materialRuntime.descriptorHandles[currentFlagId];
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(setHandle);

  VkDescriptorSet sets[] = {
      descriptorSet
  };

  // multiple descriptor sets
  vkCmdBindDescriptorSets(commandList, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          materialRuntime.layouts[currentFlagId],
                          PSOManager::PER_OBJECT_BINDING_INDEX, 1, sets, 0,
                          nullptr);
}
void VkMaterialManager::updateMaterial(SHADER_QUEUE_FLAGS queueFlag,
                                       const MaterialHandle handle,
                                       VkCommandBuffer commandList) {
  const VkMaterialRuntime &materialRuntime = getMaterialRuntime(handle);
  int queueFlagInt = static_cast<int>(queueFlag);
  int currentFlagId = static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  const SHADER_TYPE_FLAGS type =
      getTypeFlags(materialRuntime.shaderQueueTypeFlags[currentFlagId]);
  switch (type) {
    case (SHADER_TYPE_FLAGS::FORWARD_PHONG): {
      updateForwardPhong(queueFlag, materialRuntime);
      break;
    }
    default: {
      assert(0 && "could not find material type");
    }
  }
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
ShaderBind VkMaterialManager::bindRSandPSO(const uint32_t shaderFlags,
                                           const VkCommandBuffer commandList) const
{
  // get type flags as int
  constexpr auto mask = static_cast<uint32_t>(~((1 << 16) - 1));
  const auto typeFlags = static_cast<uint16_t>((shaderFlags & mask) >> 16);

  ShaderBind bind;
  bool found = m_shaderTypeToShaderBind.get(typeFlags, bind);
  if (found) {
    // beginRenderPass(bind);
    vk::PSO_MANAGER->bindPSO(bind.pso, commandList);
    return bind;
  }
  assert(0 && "Could not find requested shader type for PSO /RS bind");
  return {};
}

inline VkDescriptorImageInfo getDescriptor(const TextureHandle handle) {
  return handle.isHandleValid() ? vk::TEXTURE_MANAGER->getSrvDescriptor(handle)
                                : VkDescriptorImageInfo{};
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
  matCpu.albedo = getDescriptor(handles.albedo);
  matCpu.normal = getDescriptor(handles.normal);
  matCpu.metallic = getDescriptor(handles.metallic);
  matCpu.roughness = getDescriptor(handles.roughness);
  matCpu.thickness = getDescriptor(handles.thickness);
  matCpu.separateAlpha = getDescriptor(handles.separateAlpha);
  matCpu.ao = getDescriptor(handles.ao);
  matCpu.heightMap = getDescriptor(handles.height);
  matCpu.skinHandle = skinHandle;
  matCpu.meshHandle = meshHandle;

  memcpy(&matCpu.shaderQueueTypeFlags, parse.shaderQueueTypeFlags,
         sizeof(uint32_t) * QUEUE_COUNT);

  materialData.handles.cbHandle = globals::CONSTANT_BUFFER_MANAGER->allocate(
      sizeof(Material), 0, &parse.mat);

  matCpu.cbVirtualAddress = vk::CONSTANT_BUFFER_MANAGER->getBufferDescriptor(
      materialData.handles.cbHandle);

  materialData.magicNumber = MAGIC_NUMBER_COUNTER++;
  materialData.m_materialRuntime = matCpu;

  const std::string name = getFileName(path);
  MaterialHandle handle{(materialData.magicNumber << 16) | (index)};
  m_nameToHandle.insert(name.c_str(), handle);

  // allocate the descriptor set
  constexpr auto mask = static_cast<uint32_t>(~((1 << 16) - 1));

  // looping the queues
  for (int i = 0; i < QUEUE_COUNT; ++i) {
    uint32_t queue = matCpu.shaderQueueTypeFlags[i];
    if (queue == INVALID_QUEUE_TYPE_FLAGS) {
      continue;
    }
    const auto queueType = getQueueFlags(queue);
    const auto typeFlags = static_cast<uint16_t>((queue & mask) >> 16);
    ShaderBind bind{};
    bool found = m_shaderTypeToShaderBind.get(typeFlags, bind);

    if (!found) {
      assert(0 && "could not find needed root signature");
    }
    uint32_t flags =
        parse.isStatic
            ? 0
            : graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED;

    const char *shaderTypeName =
        getStringFromShaderTypeFlag(static_cast<SHADER_TYPE_FLAGS>(typeFlags));
    const char *descriptorName =
        frameConcatenation(name.c_str(), shaderTypeName, "-");
    DescriptorHandle descriptorHandle =
        vk::DESCRIPTOR_MANAGER->allocate(bind.rs, flags, descriptorName);

    materialData.m_materialRuntime.descriptorHandles[i] = descriptorHandle;
    materialData.m_materialRuntime.layouts[i] =
        vk::PIPELINE_LAYOUT_MANAGER->getLayoutFromHandle(bind.rs);

    if (parse.isStatic) {
      // it is static lets bind the material right away
      updateMaterial(static_cast<SHADER_QUEUE_FLAGS>(getQueueFlags(queue)),
                     handle, vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
    }
  }

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
      const char *key = m_nameToHandle.getKeyAtBin(i);
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

MaterialHandle VkMaterialManager::allocateMaterial(
    const char *name, const ALLOCATE_MATERIAL_FLAGS flags,
    const char *materialsPerQueue[QUEUE_COUNT]) {
  // empty material
  uint32_t index;
  VkMaterialData &materialData =
      m_materialTextureHandles.getFreeMemoryData(index);

  for (int i = 0; i < QUEUE_COUNT; ++i) {
    if (materialsPerQueue[i] == nullptr) {
      continue;
    }
    // from the type we can get the PSO
    const char *type = materialsPerQueue[i];
    uint16_t shaderType = parseTypeFlags(type);
    ShaderBind bind{};
    bool found = m_shaderTypeToShaderBind.get(shaderType, bind);
    assert(found && "could not find requested material type");
    assert(bind.pso.isHandleValid());
    assert(bind.rs.isHandleValid());

    // allocating a descriptor set for the material
    bool isBuffered = (flags & ALLOCATE_MATERIAL_FLAG_BITS::BUFFERED) > 0;
    graphics::BINDING_TABLE_FLAGS descriptorFlags =
        isBuffered ? graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED
                   : 0;
    DescriptorHandle descriptorHandle =
        vk::DESCRIPTOR_MANAGER->allocate(bind.rs, descriptorFlags, name);

    materialData.m_materialRuntime.descriptorHandles[i] = descriptorHandle;
    materialData.m_materialRuntime.layouts[i] =
        vk::PIPELINE_LAYOUT_MANAGER->getLayoutFromHandle(bind.rs);
    materialData.m_materialRuntime.useStaticSamplers[i] =
        vk::PIPELINE_LAYOUT_MANAGER->usesStaticSamplers(bind.rs);
    SHADER_QUEUE_FLAGS queueType = static_cast<SHADER_QUEUE_FLAGS>(1 << i);
    materialData.m_materialRuntime.shaderQueueTypeFlags[i] =
        getQueueTypeFlags(queueType, shaderType);
  }
  materialData.name = persistentString(name);
  materialData.magicNumber = MAGIC_NUMBER_COUNTER++;
  MaterialHandle handle{(materialData.magicNumber << 16) | (index)};
  m_nameToHandle.insert(name, handle);
  return handle;
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

  // find the PSO, should this be part of the runtime directly?
  uint32_t flags = data.m_materialRuntime.shaderQueueTypeFlags[currentFlagId];
  SHADER_TYPE_FLAGS type = getTypeFlags(flags);
  ShaderBind bind;
  bool found = m_shaderTypeToShaderBind.get(static_cast<uint32_t>(type), bind);
  assert(found);

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
