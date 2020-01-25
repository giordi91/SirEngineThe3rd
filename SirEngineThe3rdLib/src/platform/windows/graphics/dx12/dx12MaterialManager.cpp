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
void bindDebugLinesSingleColor(const Dx12MaterialRuntime &materialRuntime,
                               ID3D12GraphicsCommandList2 *commandList) {
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
  commandList->SetGraphicsRootDescriptorTable(
      1, dx12::CONSTANT_BUFFER_MANAGER
             ->getConstantBufferDx12Handle(materialRuntime.chandle)
             .gpuHandle);

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
  BufferHandle handle = materialRuntime.dataHandle;
  dx12::BUFFER_MANAGER->bindBufferAsDescriptorTableGrahpics(handle, 2,
                                                            commandList, 0);
  // commandList->SetGraphicsRootDescriptorTable(
  //    2, materialRuntime.meshHandle.srv.gpuHandle);
}
void bindDebugPointsSingleColor(const Dx12MaterialRuntime &materialRuntime,
                                ID3D12GraphicsCommandList2 *commandList) {
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
  commandList->SetGraphicsRootDescriptorTable(
      1, dx12::CONSTANT_BUFFER_MANAGER
             ->getConstantBufferDx12Handle(materialRuntime.chandle)
             .gpuHandle);

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  BufferHandle handle = materialRuntime.dataHandle;
  dx12::BUFFER_MANAGER->bindBufferAsDescriptorTableGrahpics(handle, 2,
                                                            commandList, 0);
  // commandList->SetGraphicsRootDescriptorTable(
  //    2, materialRuntime.meshHandle.srv.gpuHandle);
}

void bindFlatDescriptorMaterial(const Dx12MaterialRuntime &materialRuntime,
                                ID3D12GraphicsCommandList2 *commandList,
                                SHADER_QUEUE_FLAGS queueFlag) {
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
  const auto queueFlagInt = static_cast<int>(queueFlag);
  const auto currentFlagId =
      static_cast<int>(log2(queueFlagInt & -queueFlagInt));

  assert(materialRuntime.m_tables[currentFlagId].isFlatRoot == true);
  // let us bind the descriptor table
  commandList->SetGraphicsRootDescriptorTable(
      1, materialRuntime.m_tables[currentFlagId].flatDescriptors[0].gpuHandle);

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  const Dx12MeshRuntime &runtime =
      dx12::MESH_MANAGER->getMeshRuntime(materialRuntime.meshHandle);
  commandList->IASetIndexBuffer(&runtime.iview);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Dx12MaterialManager::bindMaterial(
    SHADER_QUEUE_FLAGS queueFlag, const Dx12MaterialRuntime &materialRuntime,
    ID3D12GraphicsCommandList2 *commandList) {
  const auto queueFlagInt = static_cast<int>(queueFlag);
  const auto currentFlagId =
      static_cast<int>(log2(queueFlagInt & -queueFlagInt));
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
    case (SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR): {
      bindDebugLinesSingleColor(materialRuntime, commandList);
      break;
    }
    case (SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR): {
      bindDebugPointsSingleColor(materialRuntime, commandList);
      break;
    }
    case (SHADER_TYPE_FLAGS::GRASS_FORWARD): {
      bindFlatDescriptorMaterial(materialRuntime, commandList, queueFlag);
      break;
    }
    case (SHADER_TYPE_FLAGS::FORWARD_PHONG): {
      bindFlatDescriptorMaterial(materialRuntime, commandList, queueFlag);
      break;
    }
    default: {
      assert(0 && "could not find material type");
    }
  }
}

void updateForwardPhong(const MaterialData &data,
                        ID3D12GraphicsCommandList2 *commandList,
                        SHADER_QUEUE_FLAGS queueFlag) {
  const auto queueFlagInt = static_cast<int>(queueFlag);
  const auto currentFlagId =
      static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  const FlatDescriptorTable &table =
      data.m_materialRuntime.m_tables[currentFlagId];

  assert(data.m_materialRuntime.m_tables[currentFlagId].isFlatRoot == true);

  // bind mesh  no tangents for now
  const uint32_t meshFlags = POSITIONS | NORMALS | UV;
  dx12::MESH_MANAGER->bindFlatMesh(data.m_materialRuntime.meshHandle,
                                   table.flatDescriptors, meshFlags, 0);

  // now bind the texture
  dx12::TEXTURE_MANAGER->createSRV(data.handles.albedo,
                                   table.flatDescriptors[3]);

  //const uint32_t meshFlags2 = POSITIONS;
  //dx12::MESH_MANAGER->bindFlatMesh(data.m_materialRuntime.meshHandle,
  //                                 table.flatDescriptors, meshFlags, 3);
}

void Dx12MaterialManager::updateMaterial(
    SHADER_QUEUE_FLAGS queueFlag, const MaterialData &data,
    ID3D12GraphicsCommandList2 *commandList) {
  const auto queueFlagInt = static_cast<int>(queueFlag);
  const auto currentFlagId =
      static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  const SHADER_TYPE_FLAGS type =
      getTypeFlags(data.m_materialRuntime.shaderQueueTypeFlags[currentFlagId]);
  auto &materialRuntime = data.m_materialRuntime;
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
    case (SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR): {
      assert(0);
      bindDebugLinesSingleColor(materialRuntime, commandList);
      break;
    }
    case (SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR): {
      assert(0);
      bindDebugPointsSingleColor(materialRuntime, commandList);
      break;
    }
    case (SHADER_TYPE_FLAGS::GRASS_FORWARD): {
      assert(0);
      bindFlatDescriptorMaterial(materialRuntime, commandList, queueFlag);
      break;
    }
    case (SHADER_TYPE_FLAGS::FORWARD_PHONG): {
      updateForwardPhong(data, commandList, queueFlag);
      break;
    }
    default: {
      assert(0 && "could not find material type");
    }
  }
}

void Dx12MaterialManager::bindTexture(MaterialHandle handle,
                                      const TextureHandle texHandle,
                                      const uint32_t bindingIndex,
                                      SHADER_QUEUE_FLAGS queue) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const auto &data = m_materialTextureHandles.getConstRef(index);

  const auto queueFlagInt = static_cast<int>(queue);
  const auto currentFlagId =
      static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  assert(data.m_materialRuntime.m_tables[currentFlagId].isFlatRoot == true);

  const FlatDescriptorTable &table =
      data.m_materialRuntime.m_tables[currentFlagId];

  dx12::TEXTURE_MANAGER->createSRV(texHandle,
                                   table.flatDescriptors[bindingIndex]);

  // dx12::DEVICE->CopyDescriptorsSimple(
  //    1, table.flatDescriptors[bindingIndex].cpuHandle, pair.cpuHandle,
  //    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Dx12MaterialManager::bindBuffer(const MaterialHandle handle,
                                     BufferHandle bufferHandle,
                                     const uint32_t bindingIndex,
                                     SHADER_QUEUE_FLAGS queue) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const auto &data = m_materialTextureHandles.getConstRef(index);

  const auto queueFlagInt = static_cast<int>(queue);
  const auto currentFlagId =
      static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  assert(data.m_materialRuntime.m_tables[currentFlagId].isFlatRoot == true);

  const FlatDescriptorTable &table =
      data.m_materialRuntime.m_tables[currentFlagId];

  dx12::BUFFER_MANAGER->createSrv(bufferHandle,
                                  table.flatDescriptors[bindingIndex]);
}

void Dx12MaterialManager::bindMaterial(
    const SHADER_QUEUE_FLAGS queueFlag, const MaterialHandle handle,
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
  bool found = m_shaderTypeToShaderBind.get(typeFlags, bind);
  if (found) {
    dx12::ROOT_SIGNATURE_MANAGER->bindGraphicsRS(bind.rs, commandList);
    dx12::PSO_MANAGER->bindPSO(bind.pso, commandList);
    return;
  }
  assert(0 && "Could not find requested shader type for PSO /RS bind");
}

void allocateDescriptorTable(FlatDescriptorTable &table, const RSHandle root) {
  // lets allocate enough descriptors to use for the descriptor table
  const uint32_t descriptorCount =
      dx12::ROOT_SIGNATURE_MANAGER->getDescriptorCount(root);
  auto *descriptors = reinterpret_cast<DescriptorPair *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(DescriptorPair) *
                                              descriptorCount+30));
  // now we have enough descriptors that we can use to bind everything
  uint32_t baseDescriptorIdx =
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->reserveDescriptors(descriptors,
                                                        descriptorCount+30);
  table.descriptorCount = descriptorCount;
  table.isFlatRoot = true;
  table.flatDescriptors = descriptors;
}

MaterialHandle Dx12MaterialManager::allocateMaterial(
    const char *name, ALLOCATE_MATERIAL_FLAGS flags,
    const char *materialsPerQueue[QUEUE_COUNT]) {
  // from the type we can get the PSO
  // TODO clean this up properly
  for (int i = 0; i < QUEUE_COUNT; ++i) {
    if (materialsPerQueue[i] == nullptr) {
      continue;
    }
    uint16_t shaderType = parseTypeFlags(materialsPerQueue[i]);
    ShaderBind bind;
    bool found = m_shaderTypeToShaderBind.get(shaderType, bind);
    assert(found && "could not find requested material type");
    assert(bind.pso.isHandleValid() &&
           "could find the material type but no PSO, does the correct material "
           "exists for HLSL?");

    // empty material
    uint32_t index;
    MaterialData &materialData =
        m_materialTextureHandles.getFreeMemoryData(index);
    materialData.magicNumber = MAGIC_NUMBER_COUNTER++;
    materialData.m_materialRuntime.m_tables[i].isFlatRoot = false;
    materialData.m_materialRuntime.m_tables[i].descriptorCount = 0;
    materialData.m_materialRuntime.m_tables[i].flatDescriptors = nullptr;

    // now we check whether is a flat table or not, if it is
    // we allocate the corresponding descriptors
    RSHandle root = bind.rs;
    bool isFlatRoot = dx12::ROOT_SIGNATURE_MANAGER->isFlatRoot(root);
    if (isFlatRoot) {
      allocateDescriptorTable(materialData.m_materialRuntime.m_tables[i], root);
    }

    // this will be used when we need to bind the material
    materialData.m_psoHandle = bind.pso;
    materialData.m_rsHandle = bind.rs;

    // adding correct queue
    const auto queueType = static_cast<SHADER_QUEUE_FLAGS>(1 << i);
    materialData.m_materialRuntime.shaderQueueTypeFlags[i] =
        getQueueTypeFlags(queueType, shaderType);

    const MaterialHandle handle{(materialData.magicNumber << 16) | (index)};
    m_nameToHandle.insert(name, handle);
    return handle;
  }
  assert(0);
  return {};
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

  // check if is flat material
  for (uint32_t i = 0; i < QUEUE_COUNT; ++i) {
    if (parse.shaderQueueTypeFlags[i] == INVALID_QUEUE_TYPE_FLAGS) {
      continue;
    }
    // get the type of shader
    const SHADER_TYPE_FLAGS type = getTypeFlags(parse.shaderQueueTypeFlags[i]);
    // get the rs
    ShaderBind bind;
    bool found =
        m_shaderTypeToShaderBind.get(static_cast<uint16_t>(type), bind);
    assert(found);
    assert(bind.pso.isHandleValid());
    assert(bind.rs.isHandleValid());

    // now that we have the RS we can check whether is a flat material or not
    bool isFlatRoot = dx12::ROOT_SIGNATURE_MANAGER->isFlatRoot(bind.rs);
    if (!isFlatRoot) {
      continue;
    }

    // we have a flat material we need to deal with it accordingly
    allocateDescriptorTable(materialData.m_materialRuntime.m_tables[i],
                            bind.rs);

    // we have a flat root but we actually need to setup the data
    SHADER_QUEUE_FLAGS queueFlags =
        static_cast<SHADER_QUEUE_FLAGS>(parse.shaderQueueTypeFlags[i]);
    updateMaterial(queueFlags, materialData, nullptr);
  }

  MaterialHandle handle{(materialData.magicNumber << 16) | (index)};

  const std::string name = getFileName(path);
  m_nameToHandle.insert(name.c_str(), handle);

  return handle;
}

void Dx12MaterialManager::bindMaterial(const MaterialHandle handle,
                                       SHADER_QUEUE_FLAGS queue) {
  // TODO use the queue flags once we re-designed the descriptor handling ofr
  // dx12
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const auto &data = m_materialTextureHandles.getConstRef(index);
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  dx12::PSO_MANAGER->bindPSO(data.m_psoHandle, commandList);
  ID3D12RootSignature *rs =
      dx12::ROOT_SIGNATURE_MANAGER->getRootSignatureFromHandle(data.m_rsHandle);
  commandList->SetGraphicsRootSignature(rs);

  const auto queueFlagInt = static_cast<int>(queue);
  const auto currentFlagId =
      static_cast<int>(log2(queueFlagInt & -queueFlagInt));

  assert(data.m_materialRuntime.m_tables[currentFlagId].isFlatRoot == true);
  // let us bind the descriptor table
  commandList->SetGraphicsRootDescriptorTable(
      1, data.m_materialRuntime.m_tables[currentFlagId]
             .flatDescriptors[0]
             .gpuHandle);
}

void Dx12MaterialManager::free(const MaterialHandle handle) {
  // TODO properly cleanup the resources
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const auto &data = m_materialTextureHandles.getConstRef(index);
  m_materialTextureHandles.free(index);
}
}  // namespace SirEngine::dx12
