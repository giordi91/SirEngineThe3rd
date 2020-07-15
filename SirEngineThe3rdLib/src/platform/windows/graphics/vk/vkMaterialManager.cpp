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

  VkWriteDescriptorSet writeDescriptorSets[4] = {};

  uint32_t flags = POSITIONS | NORMALS | UV;
  VkDescriptorBufferInfo bufferInfo[3] = {};
  vk::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, writeDescriptorSets,
                             descriptorSet, bufferInfo, flags, 0);
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
void bindForwardPhong(uint32_t queueId,
                      const VkMaterialRuntime &materialRuntime,
                      VkCommandBuffer commandList) {
  assert(0);
  // DescriptorHandle setHandle = materialRuntime.descriptorHandles[queueId];
  // VkDescriptorSet descriptorSet =
  //    vk::DESCRIPTOR_MANAGER->getDescriptorSet(setHandle);

  // VkDescriptorSet sets[] = {
  //    vk::PER_FRAME_DESCRIPTOR_SET[globals::CURRENT_FRAME], descriptorSet,
  //    vk::STATIC_SAMPLER_DESCRIPTOR_SET};
  //// multiple descriptor sets
  // vkCmdBindDescriptorSets(commandList, VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                        vk::PIPELINE_LAYOUT, 0, 3, sets, 0, nullptr);
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

  DescriptorHandle setHandle = materialRuntime.descriptorHandles[currentFlagId];
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(setHandle);

  VkDescriptorSet sets[] = {
      // vk::DESCRIPTOR_MANAGER->getDescriptorSet(PER_FRAME_DATA_HANDLE),
      descriptorSet
      //, vk::STATIC_SAMPLERS_DESCRIPTOR_SET
  };
  uint32_t setsToBind =
      materialRuntime.useStaticSamplers[currentFlagId] ? 3 : 2;
  // multiple descriptor sets
  vkCmdBindDescriptorSets(commandList, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          materialRuntime.layouts[currentFlagId],
                          PSOManager::PER_OBJECT_BINDING_INDEX, 1, sets, 0,
                          nullptr);
  /*
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
    bindForwardPhong(currentFlagId, materialRuntime, commandList);
    break;
  }
  default: {
    assert(0 && "could not find material type");
  }
  }
  */
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
void VkMaterialManager::bindRSandPSO(const uint32_t shaderFlags,
                                     VkCommandBuffer commandList) {
  // get type flags as int
  constexpr auto mask = static_cast<uint32_t>(~((1 << 16) - 1));
  const auto typeFlags = static_cast<uint16_t>((shaderFlags & mask) >> 16);

  ShaderBind bind;
  bool found = m_shaderTypeToShaderBind.get(typeFlags, bind);
  if (found) {
    // beginRenderPass(bind);
    vk::PSO_MANAGER->bindPSO(bind.pso, commandList);
    return;
  }
  assert(0 && "Could not find requested shader type for PSO /RS bind");
  return;
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
