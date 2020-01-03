#include "platform/windows/graphics/dx12/dx12MaterialManager.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/skinClusterManager.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/dx12/dx12BufferManager.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/dx12MeshManager.h"
#include "platform/windows/graphics/dx12/dx12PSOManager.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"

namespace SirEngine::dx12 {

void bindPBR(const Dx12MaterialRuntime &materialRuntime,
             ID3D12GraphicsCommandList2 *commandList) {

  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

  commandList->SetGraphicsRootConstantBufferView(
      1, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(2, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.metallic);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.roughness);
}

void bindSkinning(const Dx12MaterialRuntime &materialRuntime,
                  ID3D12GraphicsCommandList2 *commandList) {
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
  // dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

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
}
void bindSkin(const Dx12MaterialRuntime &materialRuntime,
              ID3D12GraphicsCommandList2 *commandList) {
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
}
void bindSkinSkinning(const Dx12MaterialRuntime &materialRuntime,
                      ID3D12GraphicsCommandList2 *commandList) {
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
}
void bindForwardPBR(const Dx12MaterialRuntime &materialRuntime,
                    ID3D12GraphicsCommandList2 *commandList) {
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
}
void bindForwardPhongAlphaCutout(const Dx12MaterialRuntime &materialRuntime,
                                 ID3D12GraphicsCommandList2 *commandList) {
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
}

void bindParallaxPBR(const Dx12MaterialRuntime &materialRuntime,
                     ID3D12GraphicsCommandList2 *commandList) {
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
}

void bindForwardPhongAlphaCutoutSkin(const Dx12MaterialRuntime &materialRuntime,
                                     ID3D12GraphicsCommandList2 *commandList) {
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
  // dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

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
}
void bindHair(const Dx12MaterialRuntime &materialRuntime,
              ID3D12GraphicsCommandList2 *commandList) {

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
}
void bindHairSkin(const Dx12MaterialRuntime &materialRuntime,
                  ID3D12GraphicsCommandList2 *commandList) {
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
  // dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

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
}

void bindShadowSkin(const Dx12MaterialRuntime &materialRuntime,
                    ID3D12GraphicsCommandList2 *commandList) {
  const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
  const auto address =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

  commandList->SetGraphicsRootConstantBufferView(1, address);
  // need to bind the skinning data
  const SkinHandle skHandle = materialRuntime.skinHandle;
  const SkinData &data = globals::SKIN_MANAGER->getSkinData(skHandle);
  // now we have both static buffers, influences and weights
  // dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

  // frame, binding material should not worry about upload
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.influencesBuffer, 2,
                                                commandList);
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.weightsBuffer, 3,
                                                commandList);
  // binding skinning data
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.matricesBuffer, 4,
                                                commandList);

  dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                               MeshAttributeFlags::POSITIONS, 5);
  // TODO HARDCODED stencil value might have to think of a nice way to handle
  // this
  commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::CLEAR));
}

void Dx12MaterialManager::bindMaterial(
    SHADER_QUEUE_FLAGS queueFlag, const Dx12MaterialRuntime &materialRuntime,
    ID3D12GraphicsCommandList2 *commandList) {
  int queueFlagInt = static_cast<int>(queueFlag);
  int currentFlagId = static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  const SHADER_TYPE_FLAGS type =
      getTypeFlags(materialRuntime.shaderQueueTypeFlags[currentFlagId]);
  switch (type) {
  case (SHADER_TYPE_FLAGS::PBR): {
    bindPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SKIN): {
    bindSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PBR): {
    bindForwardPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT): {
    bindForwardPhongAlphaCutout(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::HAIR): {
    bindHair(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SKINCLUSTER): {
    bindSkinning(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SKINSKINCLUSTER): {
    bindSkinSkinning(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT_SKIN): {
    bindForwardPhongAlphaCutoutSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::HAIRSKIN): {
    bindHairSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PARALLAX): {
    bindParallaxPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SHADOW_SKIN_CLUSTER): {
    bindShadowSkin(materialRuntime, commandList);
    break;
  }
  default: {
    assert(0 && "could not find material type");
  }
  }
}

void Dx12MaterialManager::bindTexture(MaterialHandle,
                                      const TextureHandle texHandle,
                                      const uint32_t bindingIndex) {
  dx12::DescriptorPair pair = dx12::TEXTURE_MANAGER->getSRVDx12(texHandle);

  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  commandList->SetGraphicsRootDescriptorTable(bindingIndex, pair.gpuHandle);
}

void Dx12MaterialManager::bindMaterial(
    SHADER_QUEUE_FLAGS queueFlag, const MaterialHandle handle,
    ID3D12GraphicsCommandList2 *commandList) {

  const Dx12MaterialRuntime &materialRuntime = getMaterialRuntime(handle);
  bindMaterial(queueFlag, materialRuntime, commandList);
}

void Dx12MaterialManager::bindRSandPSO(
    const uint32_t shaderFlags, ID3D12GraphicsCommandList2 *commandList) {
  // get type flags as int
  constexpr auto mask = static_cast<uint32_t>(~((1 << 16) - 1));
  const auto typeFlags = static_cast<uint16_t>((shaderFlags & mask) >> 16);

  ShaderBind bind;
  bool found = m_shderTypeToShaderBind.get(typeFlags, bind);
  if (found) {
    dx12::ROOT_SIGNATURE_MANAGER->bindGraphicsRS(bind.rs, commandList);
    dx12::PSO_MANAGER->bindPSO(bind.pso, commandList);
    return;
  }
  assert(0 && "Could not find requested shader type for PSO /RS bind");
}

MaterialHandle Dx12MaterialManager::allocateMaterial(const char *type, const char *name,
                                          ALLOCATE_MATERIAL_FLAGS ) {
  // from the type we can get the PSO
  uint16_t shaderType = parseTypeFlags(type);
  ShaderBind bind;
  bool found = m_shderTypeToShaderBind.get(shaderType, bind);
  assert(found && "could not find requested material type");

  // TODO for now, not much is happening here, because we have a crappy binding
  // system where each resource is bound with independent descriptor table, when
  // we change this, here we will be doing an update of the binding table

  // empty material
  uint32_t index;
  MaterialData &materialData =
      m_materialTextureHandles.getFreeMemoryData(index);
  materialData.magicNumber = MAGIC_NUMBER_COUNTER++;

  // this will be used when we need to bind the material
  materialData.m_psoHandle= bind.pso;
  materialData.m_rsHandle= bind.rs;

  MaterialHandle handle{(materialData.magicNumber << 16) | (index)};
  m_nameToHandle.insert(name, handle);
  return handle;
}

MaterialHandle Dx12MaterialManager::loadMaterial(const char *path,
                                                 const MeshHandle meshHandle,
                                                 const SkinHandle skinHandle) {
  PrelinaryMaterialParse parse = parseMaterial(path, meshHandle, skinHandle);

  uint32_t index;
  MaterialData &materialData =
      m_materialTextureHandles.getFreeMemoryData(index);

  materialData.m_material = parse.mat;
  materialData.handles = parse.handles;

  const MaterialDataHandles &handles = materialData.handles;

  if (handles.albedo.handle != 0) {
    materialData.albedoSrv = dx12::TEXTURE_MANAGER->getSRVDx12(handles.albedo);
  }
  if (handles.normal.handle != 0) {
    materialData.normalSrv = dx12::TEXTURE_MANAGER->getSRVDx12(handles.normal);
  }
  if (handles.metallic.handle != 0) {
    materialData.metallicSrv =
        dx12::TEXTURE_MANAGER->getSRVDx12(handles.metallic);
  }
  if (handles.roughness.handle != 0) {
    materialData.roughnessSrv =
        dx12::TEXTURE_MANAGER->getSRVDx12(handles.roughness);
  }
  if (handles.thickness.handle != 0) {
    materialData.thicknessSrv =
        dx12::TEXTURE_MANAGER->getSRVDx12(handles.thickness);
  }
  if (handles.separateAlpha.handle != 0) {
    materialData.separateAlphaSrv =
        dx12::TEXTURE_MANAGER->getSRVDx12(handles.separateAlpha);
  }
  if (handles.ao.handle != 0) {
    materialData.aoSrv = dx12::TEXTURE_MANAGER->getSRVDx12(handles.ao);
  }
  if (handles.height.handle != 0) {
    materialData.heightSrv = dx12::TEXTURE_MANAGER->getSRVDx12(handles.height);
  }
  Dx12MaterialRuntime matCpu{};
  matCpu.albedo = materialData.albedoSrv.gpuHandle;
  matCpu.normal = materialData.normalSrv.gpuHandle;
  matCpu.metallic = materialData.metallicSrv.gpuHandle;
  matCpu.roughness = materialData.roughnessSrv.gpuHandle;
  matCpu.thickness = materialData.thicknessSrv.gpuHandle;
  matCpu.separateAlpha = materialData.separateAlphaSrv.gpuHandle;
  matCpu.ao = materialData.aoSrv.gpuHandle;
  matCpu.skinHandle = skinHandle;
  matCpu.heightMap = materialData.heightSrv.gpuHandle;
  matCpu.meshHandle = meshHandle;

  memcpy(&matCpu.shaderQueueTypeFlags, parse.shaderQueueTypeFlags,
         sizeof(uint32_t) * 4);

  // we need to allocate  constant buffer
  // TODO should this be static constant buffer? investigate
  materialData.handles.cbHandle =
      globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(Material),
                                                        &parse.mat);

  matCpu.cbVirtualAddress = dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(
      materialData.handles.cbHandle);

  materialData.magicNumber = MAGIC_NUMBER_COUNTER++;
  materialData.m_materialRuntime = matCpu;

  MaterialHandle handle{(materialData.magicNumber << 16) | (index)};

  const std::string name = getFileName(path);
  m_nameToHandle.insert(name.c_str(), handle);

  return handle;
}

void Dx12MaterialManager::bindMaterial(MaterialHandle handle) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const auto &data = m_materialTextureHandles.getConstRef(index);
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  dx12::PSO_MANAGER->bindPSO(data.m_psoHandle, commandList);
  ID3D12RootSignature *rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromHandle(data.m_rsHandle);
  commandList->SetGraphicsRootSignature(rs);
}

void Dx12MaterialManager::free(MaterialHandle handle) {
  // TODO properly cleanup the resources
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const auto &data = m_materialTextureHandles.getConstRef(index);
  m_materialTextureHandles.free(index);
}
} // namespace SirEngine::dx12
