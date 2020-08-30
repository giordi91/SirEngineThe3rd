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

void bindFlatDescriptorMaterial(const Dx12MaterialRuntime &materialRuntime,
                                ID3D12GraphicsCommandList2 *commandList,
                                SHADER_QUEUE_FLAGS queueFlag,
                                const RSHandle rsHandle) {
  const auto queueFlagInt = static_cast<int>(queueFlag);
  const auto currentFlagId =
      static_cast<int>(log2(queueFlagInt & -queueFlagInt));

  assert(materialRuntime.m_tables[currentFlagId].isFlatRoot == true);
  // let us bind the descriptor table
  // we look up the binding slot for the per object based on root signature
  int bindSlot = dx12::ROOT_SIGNATURE_MANAGER->getBindingSlot(
      rsHandle, PSOManager::PER_OBJECT_BINDING_INDEX);
  assert(bindSlot != -1);
  commandList->SetGraphicsRootDescriptorTable(
      bindSlot,
      materialRuntime.m_tables[currentFlagId].flatDescriptors[0].gpuHandle);

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  const Dx12MeshRuntime &runtime =
      dx12::MESH_MANAGER->getMeshRuntime(materialRuntime.meshHandle);
  commandList->IASetIndexBuffer(&runtime.iview);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Dx12MaterialManager::bindMaterial(
    SHADER_QUEUE_FLAGS queueFlag, const Dx12MaterialRuntime &materialRuntime,
    ID3D12GraphicsCommandList2 *commandList) const {
  const auto queueFlagInt = static_cast<int>(queueFlag);
  const auto currentFlagId =
      static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  const SHADER_TYPE_FLAGS type =
      getTypeFlags(materialRuntime.shaderQueueTypeFlags[currentFlagId]);

  ShaderBind bind;
  bool found = m_shaderTypeToShaderBind.get(static_cast<uint16_t>(type), bind);
  assert(type == SHADER_TYPE_FLAGS::FORWARD_PHONG);
  bindFlatDescriptorMaterial(materialRuntime, commandList, queueFlag, bind.rs);
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
  const uint32_t meshFlags = POSITIONS | NORMALS | UV | TANGENTS;
  dx12::MESH_MANAGER->bindFlatMesh(data.m_materialRuntime.meshHandle,
                                   table.flatDescriptors, meshFlags, 0);

  // now bind the texture
  // HARDCODED albedo
  dx12::TEXTURE_MANAGER->createSRV(data.handles.albedo,
                                   table.flatDescriptors[4], false);
  dx12::TEXTURE_MANAGER->createSRV(data.handles.normal,
                                   table.flatDescriptors[5], false);
  dx12::TEXTURE_MANAGER->createSRV(data.handles.metallic,
                                   table.flatDescriptors[6], false);
  dx12::TEXTURE_MANAGER->createSRV(data.handles.roughness,
                                   table.flatDescriptors[7], false);
}

void Dx12MaterialManager::updateMaterial(
    SHADER_QUEUE_FLAGS queueFlag, const MaterialData &data,
    ID3D12GraphicsCommandList2 *commandList) const {
  const auto queueFlagInt = static_cast<int>(queueFlag);
  const auto currentFlagId =
      static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  const SHADER_TYPE_FLAGS type =
      getTypeFlags(data.m_materialRuntime.shaderQueueTypeFlags[currentFlagId]);
  auto &materialRuntime = data.m_materialRuntime;

  ShaderBind bind;
  bool found = m_shaderTypeToShaderBind.get(static_cast<uint16_t>(type), bind);
  switch (type) {
    case (SHADER_TYPE_FLAGS::FORWARD_PHONG): {
      updateForwardPhong(data, commandList, queueFlag);
      break;
    }
    default: {
      assert(0 && "could not find material type");
    }
  }
}

void Dx12MaterialManager::bindTexture(const MaterialHandle matHandle,
                                      const TextureHandle texHandle,
                                      const uint32_t descriptorIndex,
                                      const uint32_t bindingIndex,
                                      SHADER_QUEUE_FLAGS queue,
                                      const bool isCubeMap) {
  assertMagicNumber(matHandle);
  uint32_t index = getIndexFromHandle(matHandle);
  const auto &data = m_materialTextureHandles.getConstRef(index);

  const auto queueFlagInt = static_cast<int>(queue);
  const auto currentFlagId =
      static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  assert(data.m_materialRuntime.m_tables[currentFlagId].isFlatRoot == true);

  const FlatDescriptorTable &table =
      data.m_materialRuntime.m_tables[currentFlagId];

  auto &descriptor = table.flatDescriptors[descriptorIndex];
  dx12::TEXTURE_MANAGER->createSRV(texHandle, descriptor, isCubeMap);
}

void Dx12MaterialManager::bindMesh(const MaterialHandle handle,
                                   const MeshHandle meshHandle,
                                   const uint32_t descriptorIndex,
                                   const uint32_t bindingIndex,
                                   const uint32_t meshBindFlags,
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

  dx12::MESH_MANAGER->bindFlatMesh(meshHandle, table.flatDescriptors,
                                   meshBindFlags, descriptorIndex);
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

void Dx12MaterialManager::bindConstantBuffer(
    const MaterialHandle handle, const ConstantBufferHandle bufferHandle,
    const uint32_t descriptorIndex, const uint32_t bindingIndex,
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

  dx12::CONSTANT_BUFFER_MANAGER->createSrv(
      bufferHandle, table.flatDescriptors[descriptorIndex]);
}

ShaderBind Dx12MaterialManager::bindRSandPSO(
    const uint32_t shaderFlags, ID3D12GraphicsCommandList2 *commandList) const {
  // get type flags as int
  constexpr auto mask = static_cast<uint32_t>(~((1 << 16) - 1));
  const auto typeFlags = static_cast<uint16_t>((shaderFlags & mask) >> 16);

  ShaderBind bind;
  bool found = m_shaderTypeToShaderBind.get(typeFlags, bind);
  if (found) {
    dx12::ROOT_SIGNATURE_MANAGER->bindGraphicsRS(bind.rs, commandList);
    dx12::PSO_MANAGER->bindPSO(bind.pso, commandList);
    return bind;
  }
  assert(0 && "Could not find requested shader type for PSO /RS bind");
  return {};
}

void allocateDescriptorTable(FlatDescriptorTable &table, const RSHandle root) {
  // lets allocate enough descriptors to use for the descriptor table
  const uint32_t descriptorCount =
      dx12::ROOT_SIGNATURE_MANAGER->getDescriptorCount(root);
  // Fix magic number...
  auto *descriptors = reinterpret_cast<DescriptorPair *>(
      globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(DescriptorPair) * descriptorCount + 30));
  // now we have enough descriptors that we can use to bind everything
  dx12::GLOBAL_CBV_SRV_UAV_HEAP->reserveDescriptors(descriptors,
                                                    descriptorCount);
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
  PreliminaryMaterialParse parse = parseMaterial(path, meshHandle, skinHandle);

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
  materialData.handles.cbHandle = globals::CONSTANT_BUFFER_MANAGER->allocate(
      sizeof(Material), 0, &parse.mat);

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
  auto gpuHandle = data.m_materialRuntime.m_tables[currentFlagId]
                       .flatDescriptors[0]
                       .gpuHandle;

  commandList->SetGraphicsRootDescriptorTable(1, gpuHandle);

  TOPOLOGY_TYPE topology = dx12::PSO_MANAGER->getTopology(data.m_psoHandle);
  switch (topology) {
    case (TOPOLOGY_TYPE::TRIANGLE): {
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      break;
    }
    case TOPOLOGY_TYPE::UNDEFINED: {
      assert(0 && "trying to bind undefined topology");
      return;
    }
    case TOPOLOGY_TYPE::LINE: {
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
      break;
    }
    case TOPOLOGY_TYPE::LINE_STRIP: {
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
      break;
    }
    case TOPOLOGY_TYPE::TRIANGLE_STRIP: {
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      break;
    }
    default:;
  }
}

void Dx12MaterialManager::free(const MaterialHandle handle) {
  // TODO properly cleanup the resources
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  m_materialTextureHandles.free(index);
}
}  // namespace SirEngine::dx12
