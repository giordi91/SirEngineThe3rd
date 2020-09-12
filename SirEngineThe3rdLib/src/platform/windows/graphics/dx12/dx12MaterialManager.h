#pragma once

#include <cassert>

#include "SirEngine/handle.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/memory/cpu/stringHashMap.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"

namespace SirEngine::dx12 {

struct FlatDescriptorTable {
  uint32_t isFlatRoot : 1;
  uint32_t descriptorCount : 15;
  uint32_t padding : 16;
  DescriptorPair *flatDescriptors;
};

struct Dx12MaterialRuntime final {
  ConstantBufferHandle chandle;
  BufferHandle dataHandle;
  D3D12_GPU_VIRTUAL_ADDRESS cbVirtualAddress;
  D3D12_GPU_DESCRIPTOR_HANDLE albedo;
  D3D12_GPU_DESCRIPTOR_HANDLE normal;
  D3D12_GPU_DESCRIPTOR_HANDLE metallic;
  D3D12_GPU_DESCRIPTOR_HANDLE roughness;
  D3D12_GPU_DESCRIPTOR_HANDLE thickness;
  D3D12_GPU_DESCRIPTOR_HANDLE separateAlpha;
  D3D12_GPU_DESCRIPTOR_HANDLE heightMap;
  D3D12_GPU_DESCRIPTOR_HANDLE ao;
  uint32_t shaderQueueTypeFlags[MaterialManager::QUEUE_COUNT] = {
      INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
      INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
      INVALID_QUEUE_TYPE_FLAGS};
  ShaderBind shaderQueueTypeFlags2[MaterialManager::QUEUE_COUNT] = {};
  SkinHandle skinHandle;
  MeshHandle meshHandle;
  FlatDescriptorTable m_tables[MaterialManager::QUEUE_COUNT];
};

struct MaterialData {
  MaterialDataHandles handles;
  dx12::DescriptorPair albedoSrv;
  dx12::DescriptorPair normalSrv;
  dx12::DescriptorPair metallicSrv;
  dx12::DescriptorPair roughnessSrv;
  dx12::DescriptorPair thicknessSrv;
  dx12::DescriptorPair separateAlphaSrv;
  dx12::DescriptorPair aoSrv;
  dx12::DescriptorPair heightSrv;
  uint32_t magicNumber;
  Material m_material;
  Dx12MaterialRuntime m_materialRuntime;
  PSOHandle m_psoHandle;
  RSHandle m_rsHandle;
};

class Dx12MaterialManager final : public MaterialManager {
 public:
  Dx12MaterialManager()
      : MaterialManager(),
        m_nameToHandle(RESERVE_SIZE),
        m_materialTextureHandles(RESERVE_SIZE){};
  ~Dx12MaterialManager() = default;
  void inititialize() override{};
  void cleanup() override{};

  void bindMaterial(MaterialHandle handle, SHADER_QUEUE_FLAGS queue) override;

  void bindTexture(const MaterialHandle matHandle,
                   const TextureHandle texHandle,
                   const uint32_t descriptorIndex, const uint32_t bindingIndex,
                   SHADER_QUEUE_FLAGS queue, const bool isCubeMap) override;
  void bindMesh(const MaterialHandle handle, const MeshHandle meshHandle,
                const uint32_t descriptorIndex, const uint32_t bindingIndex,
                const uint32_t meshBindFlags,
                SHADER_QUEUE_FLAGS queue) override;

  void bindBuffer(MaterialHandle handle, BufferHandle bufferHandle,
                  uint32_t bindingIndex, SHADER_QUEUE_FLAGS queue) override;
  void bindConstantBuffer(MaterialHandle handle,
                          ConstantBufferHandle bufferHandle,
                          const uint32_t descriptorIndex,
                          const uint32_t bindingIndex,
                          SHADER_QUEUE_FLAGS queue) override;

  // TODO I am not super happy about this, there should be an easy way to get
  // access to the RS and PSO. Here this function is called inside render queue,
  // where I will need to bind the pass table. To do so i need to convert from
  // shader space to actual descriptro index, such information is held by the
  // root signature, as such I am returning that here so I can access it, not as
  // clean as I would like
  ShaderBind bindRSandPSO(const uint64_t shaderFlags,
                          const Dx12MaterialRuntime &runtime,
                          ID3D12GraphicsCommandList2 *commandList) const;
  Dx12MaterialManager(const Dx12MaterialManager &) = delete;
  Dx12MaterialManager &operator=(const Dx12MaterialManager &) = delete;

  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                    const Dx12MaterialRuntime &runtime,
                    ID3D12GraphicsCommandList2 *commandList) const;

  MaterialHandle loadMaterial(const char *path, const MeshHandle meshHandle,
                              const SkinHandle skinHandle) override;
  void free(MaterialHandle handle) override;

  const Dx12MaterialRuntime &getMaterialRuntime(const MaterialHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_materialTextureHandles.getConstRef(index).m_materialRuntime;
  }

 private:
  inline void assertMagicNumber(const MaterialHandle handle) {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(m_materialTextureHandles[idx].magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }
  void Dx12MaterialManager::updateMaterial(
      SHADER_QUEUE_FLAGS queueFlag, const MaterialData &data,
      ID3D12GraphicsCommandList2 *commandList) const;

 private:
  HashMap<const char *, MaterialHandle, hashString32> m_nameToHandle;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<MaterialData> m_materialTextureHandles;
};

}  // namespace SirEngine::dx12
