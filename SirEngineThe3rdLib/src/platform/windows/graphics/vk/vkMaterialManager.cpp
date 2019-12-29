#include "platform/windows/graphics/vk/vkMaterialManager.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/skinClusterManager.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "vk.h"
#include "vkConstantBufferManager.h"
#include "vkDescriptorManager.h"
#include "vkMeshManager.h"
#include "vkPSOManager.h"
#include "vkRootSignatureManager.h"
#include "vkTextureManager.h"

namespace SirEngine::vk {

void bindPBR(const VkMaterialRuntime &materialRuntime,
             VkCommandBuffer commandList) {

  /*
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

  commandList->SetGraphicsRootConstantBufferView(
      1, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(2, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.metallic);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.roughness);
  */
}

void bindSkinning(const VkMaterialRuntime &materialRuntime,
                  VkCommandBuffer commandList) {
  /*
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

  commandList->SetGraphicsRootConstantBufferView(
      1, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(2, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.metallic);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.roughness);

  // need to bind the skinning data
  const SkinHandle skHandle = materialRuntime.skinHandle;
  const SkinData &data = globals::SKIN_MANAGER->getSkinData(skHandle);
  // now we have both static buffers, influences and weights
  //
  dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

  // binding skinning data
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.influencesBuffer, 6,
                                                commandList);
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.weightsBuffer, 7,
                                                commandList);
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.matricesBuffer, 8,
                                                commandList);

  dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                               MeshAttributeFlags::ALL, 9);
  // bind mesh data

  // dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(.matricesBuffer, 8,
  //                                              commandList);
  */
}
void bindSkin(const VkMaterialRuntime &materialRuntime,
              VkCommandBuffer commandList) {
  /*
commandList->SetGraphicsRootConstantBufferView(
    1, materialRuntime.cbVirtualAddress);
commandList->SetGraphicsRootDescriptorTable(2, materialRuntime.albedo);
commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.normal);
commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.metallic);
commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.roughness);
// bind extra thickness map
commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.thickness);

// HARDCODED stencil value might have to think of a nice way to handle this
commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::SSSSS));
*/
}
void bindSkinSkinning(const VkMaterialRuntime &materialRuntime,
                      VkCommandBuffer commandList) {
  /*
dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
commandList->SetGraphicsRootConstantBufferView(
    1, materialRuntime.cbVirtualAddress);
commandList->SetGraphicsRootDescriptorTable(2, materialRuntime.albedo);
commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.normal);
commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.metallic);
commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.roughness);
// bind extra thickness map
commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.thickness);

// need to bind the skinning data
const SkinHandle skHandle = materialRuntime.skinHandle;
const SkinData &data = globals::SKIN_MANAGER->getSkinData(skHandle);

// now we have both static buffers, influences and weights
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.influencesBuffer, 7,
                                              commandList);
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.weightsBuffer, 8,
                                              commandList);
// binding skinning data
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.matricesBuffer, 9,
                                              commandList);
dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                             MeshAttributeFlags::ALL, 10);

// HARDCODED stencil value might have to think of a nice way to handle this
commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::SSSSS));
*/
}
void updateForwardPhong(SHADER_QUEUE_FLAGS queueFlag,
                        const MaterialHandle handle,
                        VkMaterialManager *manager) {

  const VkMaterialRuntime &materialRuntime =
      manager->getMaterialRuntime(handle);
  // Update the descriptor set with the actual descriptors matching shader
  // bindings set in the layout
  // this far we defined just what descriptor we wanted and how they were setup,
  // now we need to actually define the content of those descriptors, the actual
  // resources

  int queueFlagInt = static_cast<int>(queueFlag);
  DescriptorHandle setHandle = materialRuntime.descriptorHandles[queueFlagInt];
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(setHandle);

  VkWriteDescriptorSet writeDescriptorSets[4] = {};

  VkDescriptorBufferInfo bufferInfo[3] = {};
  vk::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, writeDescriptorSets,
                             descriptorSet, bufferInfo);
  // root, bufferInfo);

  vk::TEXTURE_MANAGER->bindTexture(materialRuntime.albedo,
                                   &writeDescriptorSets[3], descriptorSet, 3);

  // Execute the writes to update descriptors for this set
  // Note that it's also possible to gather all writes and only run updates
  // once, even for multiple sets This is possible because each
  // VkWriteDescriptorSet also contains the destination set to be updated
  // For simplicity we will update once per set instead
  // object one off update
  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 4, &writeDescriptorSets[0], 0,
                         nullptr);
}
void bindForwardPhong(const VkMaterialRuntime &materialRuntime,
                      VkCommandBuffer commandList) {
  assert(0);
}
void bindForwardPBR(const VkMaterialRuntime &materialRuntime,
                    VkCommandBuffer commandList) {
  /*
const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
const auto address =
    dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

commandList->SetGraphicsRootConstantBufferView(1, address);
commandList->SetGraphicsRootConstantBufferView(
    2, materialRuntime.cbVirtualAddress);
commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.metallic);
commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.roughness);

TextureHandle skyHandle =
    dx12::RENDERING_CONTEXT->getEnviromentMapIrradianceHandle();
commandList->SetGraphicsRootDescriptorTable(
    7, dx12::TEXTURE_MANAGER->getSRVDx12(skyHandle).gpuHandle);

TextureHandle skyRadianceHandle =
    dx12::RENDERING_CONTEXT->getEnviromentMapRadianceHandle();
commandList->SetGraphicsRootDescriptorTable(
    8, dx12::TEXTURE_MANAGER->getSRVDx12(skyRadianceHandle).gpuHandle);

TextureHandle brdfHandle = dx12::RENDERING_CONTEXT->getBrdfHandle();
commandList->SetGraphicsRootDescriptorTable(
    9, dx12::TEXTURE_MANAGER->getSRVDx12(brdfHandle).gpuHandle);
    */
}
void bindForwardPhongAlphaCutout(const VkMaterialRuntime &materialRuntime,
                                 VkCommandBuffer commandList) {
  /*
const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
const auto address =
    dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

commandList->SetGraphicsRootConstantBufferView(1, address);
commandList->SetGraphicsRootConstantBufferView(
    2, materialRuntime.cbVirtualAddress);
commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.separateAlpha);
// HARDCODED stencil value might have to think of a nice way to handle this

commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::CLEAR));
*/
}

void bindParallaxPBR(const VkMaterialRuntime &materialRuntime,
                     VkCommandBuffer commandList) {
  /*
const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
const auto address =
    dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

commandList->SetGraphicsRootConstantBufferView(1, address);
commandList->SetGraphicsRootConstantBufferView(
    2, materialRuntime.cbVirtualAddress);
commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.metallic);
commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.roughness);

TextureHandle skyHandle =
    dx12::RENDERING_CONTEXT->getEnviromentMapIrradianceHandle();
commandList->SetGraphicsRootDescriptorTable(
    7, dx12::TEXTURE_MANAGER->getSRVDx12(skyHandle).gpuHandle);

TextureHandle skyRadianceHandle =
    dx12::RENDERING_CONTEXT->getEnviromentMapRadianceHandle();
commandList->SetGraphicsRootDescriptorTable(
    8, dx12::TEXTURE_MANAGER->getSRVDx12(skyRadianceHandle).gpuHandle);

TextureHandle brdfHandle = dx12::RENDERING_CONTEXT->getBrdfHandle();
commandList->SetGraphicsRootDescriptorTable(
    9, dx12::TEXTURE_MANAGER->getSRVDx12(brdfHandle).gpuHandle);
commandList->SetGraphicsRootDescriptorTable(10, materialRuntime.heightMap);

commandList->SetGraphicsRootDescriptorTable(
    11, dx12::TEXTURE_MANAGER
            ->getSRVDx12(globals::DEBUG_FRAME_DATA->directionalShadow)
            .gpuHandle);
dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                             MeshAttributeFlags::ALL, 12);
                             */
}

void bindForwardPhongAlphaCutoutSkin(const VkMaterialRuntime &materialRuntime,
                                     VkCommandBuffer commandList) {
  /*
dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
const auto address =
    dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

commandList->SetGraphicsRootConstantBufferView(1, address);
commandList->SetGraphicsRootConstantBufferView(
    2, materialRuntime.cbVirtualAddress);
commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.separateAlpha);

// need to bind the skinning data
const SkinHandle skHandle = materialRuntime.skinHandle;
const SkinData &data = globals::SKIN_MANAGER->getSkinData(skHandle);
// now we have both static buffers, influences and weights
//
dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

// frame, binding material should not worry about upload
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.influencesBuffer, 6,
                                              commandList);
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.weightsBuffer, 7,
                                              commandList);
// binding skinning data
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.matricesBuffer, 8,
                                              commandList);
dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                             MeshAttributeFlags::ALL, 9);

// HARDCODED stencil value might have to think of a nice way to handle this
commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::CLEAR));
*/
}
void bindHair(const VkMaterialRuntime &materialRuntime,
              VkCommandBuffer commandList) {

  /*
dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
const auto address =
    dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

commandList->SetGraphicsRootConstantBufferView(1, address);
commandList->SetGraphicsRootConstantBufferView(
    2, materialRuntime.cbVirtualAddress);
commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.separateAlpha);
commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.ao);
// HARDCODED stencil value might have to think of a nice way to handle this
commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::CLEAR));
*/
}
void bindHairSkin(const VkMaterialRuntime &materialRuntime,
                  VkCommandBuffer commandList) {
  /*
const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
const auto address =
    dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

commandList->SetGraphicsRootConstantBufferView(1, address);
commandList->SetGraphicsRootConstantBufferView(
    2, materialRuntime.cbVirtualAddress);
commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.separateAlpha);
commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.ao);

// need to bind the skinning data
const SkinHandle skHandle = materialRuntime.skinHandle;
const SkinData &data = globals::SKIN_MANAGER->getSkinData(skHandle);
// now we have both static buffers, influences and weights
//
dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

// frame, binding material should not worry about upload
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.influencesBuffer, 7,
                                              commandList);
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.weightsBuffer, 8,
                                              commandList);

// binding skinning matrices
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.matricesBuffer, 9,
                                              commandList);

dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                             MeshAttributeFlags::ALL, 10);

// HARDCODED stencil value might have to think of a nice way to handle this
commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::CLEAR));
*/
}

void bindShadowSkin(const VkMaterialRuntime &materialRuntime,
                    VkCommandBuffer commandList) {
  /*
const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
const auto address =
    dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

commandList->SetGraphicsRootConstantBufferView(0, address);
// need to bind the skinning data
const SkinHandle skHandle = materialRuntime.skinHandle;
const SkinData &data = globals::SKIN_MANAGER->getSkinData(skHandle);
// now we have both static buffers, influences and weights
//
dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

// frame, binding material should not worry about upload
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.influencesBuffer, 1,
                                              commandList);
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.weightsBuffer, 2,
                                              commandList);
// binding skinning data
dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.matricesBuffer, 3,
                                              commandList);

dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                             MeshAttributeFlags::POSITIONS, 4);
// TODO HARDCODED stencil value might have to think of a nice way to handle
// this
commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::CLEAR));
*/
}

void VkMaterialManager::bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                                     const VkMaterialRuntime &materialRuntime,
                                     VkCommandBuffer commandList) {
  int queueFlagInt = static_cast<int>(queueFlag);
  int currentFlagId = static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  const SHADER_TYPE_FLAGS type =
      getTypeFlags(materialRuntime.shaderQueueTypeFlags[currentFlagId]);
  switch (type) {
  case (SHADER_TYPE_FLAGS::PBR): {
    assert(0);
    bindPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SKIN): {
    assert(0);
    bindSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PBR): {
    assert(0);
    bindForwardPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT): {
    assert(0);
    bindForwardPhongAlphaCutout(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::HAIR): {
    assert(0);
    bindHair(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SKINCLUSTER): {
    assert(0);
    bindSkinning(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SKINSKINCLUSTER): {
    assert(0);
    bindSkinSkinning(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT_SKIN): {
    assert(0);
    bindForwardPhongAlphaCutoutSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::HAIRSKIN): {
    assert(0);
    bindHairSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PARALLAX): {
    assert(0);
    bindParallaxPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SHADOW_SKIN_CLUSTER): {
    assert(0);
    bindShadowSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PHONG): {
    bindForwardPhong(materialRuntime, commandList);
    break;
  }
  default: {
    assert(0 && "could not find material type");
  }
  }
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
  case (SHADER_TYPE_FLAGS::PBR): {
    assert(0);
    bindPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SKIN): {
    assert(0);
    bindSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PBR): {
    assert(0);
    bindForwardPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT): {
    assert(0);
    bindForwardPhongAlphaCutout(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::HAIR): {
    assert(0);
    bindHair(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SKINCLUSTER): {
    assert(0);
    bindSkinning(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SKINSKINCLUSTER): {
    assert(0);
    bindSkinSkinning(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT_SKIN): {
    assert(0);
    bindForwardPhongAlphaCutoutSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::HAIRSKIN): {
    assert(0);
    bindHairSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PARALLAX): {
    assert(0);
    bindParallaxPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SHADOW_SKIN_CLUSTER): {
    assert(0);
    bindShadowSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PHONG): {
    updateForwardPhong(queueFlag, handle, this);
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

void VkMaterialManager::bindRSandPSO(const uint32_t shaderFlags,
                                     VkCommandBuffer commandList) {
  // get type flags as int
  constexpr auto mask = static_cast<uint32_t>(~((1 << 16) - 1));
  const auto typeFlags = static_cast<uint16_t>((shaderFlags & mask) >> 16);

  ShaderBind bind;
  bool found = m_shderTypeToShaderBind.get(typeFlags, bind);
  if (found) {
    vk::PIPELINE_LAYOUT_MANAGER->bindGraphicsRS(bind.rs, commandList);
    vk::PSO_MANAGER->bindPSO(bind.pso, commandList);
    return;
  }
  assert(0 && "Could not find requested shader type for PSO /RS bind");
}

inline VkDescriptorImageInfo getDescriptor(const TextureHandle handle) {
  return handle.isHandleValid()
             ? vk::TEXTURE_MANAGER->getTextureDescriptor(handle)
             : VkDescriptorImageInfo{};
}

MaterialHandle VkMaterialManager::loadMaterial(const char *path,
                                               const MeshHandle meshHandle,
                                               const SkinHandle skinHandle) {
  PrelinaryMaterialParse parse = parseMaterial(path, meshHandle, skinHandle);

  uint32_t index;
  MaterialData &materialData =
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
         sizeof(uint32_t) * 4);

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
  for (int i = 0; i < 4; ++i) {
    uint32_t queue = matCpu.shaderQueueTypeFlags[i];
    if (queue == INVALID_QUEUE_TYPE_FLAGS) {
      continue;
    }
    const auto typeFlags = static_cast<uint16_t>((queue & mask) >> 16);
    ShaderBind bind{};
    bool found = m_shderTypeToShaderBind.get(typeFlags, bind);

    if (!found) {
      assert(0 && "could not find needed root signature");
    }
    uint32_t flags =
        parse.isStatic ? 0 : VkDescriptorManager::DESCRIPTOR_FLAGS::BUFFERED;

    const char *shaderTypeName =
        getStringFromShaderTypeFlag(static_cast<SHADER_TYPE_FLAGS>(typeFlags));
    const char *descriptorName =
        frameConcatenation(name.c_str(), shaderTypeName, "-");
    DescriptorHandle descriptorHandle =
        vk::DESCRIPTOR_MANAGER->allocate(bind.rs, flags, descriptorName);

    materialData.m_materialRuntime.descriptorHandles[i] = descriptorHandle;

    if (parse.isStatic) {
      // it is static lets bind the material right away
      updateMaterial(static_cast<SHADER_QUEUE_FLAGS>(getQueueFlags(queue)),
                     handle, vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
    }
  }

  return handle;
}

} // namespace SirEngine::vk
