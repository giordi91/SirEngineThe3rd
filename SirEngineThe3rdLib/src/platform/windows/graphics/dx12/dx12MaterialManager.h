#pragma once

#include <cassert>

#include "SirEngine/handle.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/memory/stringHashMap.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"

namespace SirEngine::dx12 {

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
  SkinHandle skinHandle;
  MeshHandle meshHandle;
};

struct FlatDescriptorTalbe {
  uint32_t isFlatRoot : 1;
  uint32_t descriptorCount : 15;
  uint32_t pading : 16;
  DescriptorPair *flatDescriptors;
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
  FlatDescriptorTalbe m_tables[MaterialManager::QUEUE_COUNT];
};

class Dx12MaterialManager final : public MaterialManager {
 public:
  Dx12MaterialManager()
      : MaterialManager(RESERVE_SIZE),
        m_nameToHandle(RESERVE_SIZE),
        m_materialTextureHandles(RESERVE_SIZE){};
  ~Dx12MaterialManager() = default;
  void inititialize() override{};
  void cleanup() override{};
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag, const MaterialHandle handle,
                    ID3D12GraphicsCommandList2 *commandList);
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                    const Dx12MaterialRuntime &runtime,
                    ID3D12GraphicsCommandList2 *commandList);
  void bindTexture(MaterialHandle matHandle, TextureHandle texHandle,
                   uint32_t bindingIndex, SHADER_QUEUE_FLAGS queue) override;
  void bindBuffer(MaterialHandle matHandle, BufferHandle texHandle,
                  uint32_t bindingIndex, SHADER_QUEUE_FLAGS queue) override;

  void bindRSandPSO(uint32_t shaderFlags,
                    ID3D12GraphicsCommandList2 *commandList);
  Dx12MaterialManager(const Dx12MaterialManager &) = delete;
  Dx12MaterialManager &operator=(const Dx12MaterialManager &) = delete;

  MaterialHandle allocateMaterial(
      const char *name, ALLOCATE_MATERIAL_FLAGS flags,
      const char *materialsPerQueue[QUEUE_COUNT]) override;
  MaterialHandle loadMaterial(const char *path, const MeshHandle meshHandle,
                              const SkinHandle skinHandle) override;
  void bindMaterial(MaterialHandle handle, SHADER_QUEUE_FLAGS queue) override;
  void free(MaterialHandle handle) override;

  const Dx12MaterialRuntime &getMaterialRuntime(const MaterialHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_materialTextureHandles.getConstRef(index).m_materialRuntime;
  }

 private:
  inline void assertMagicNumber(const MaterialHandle handle) {
    const uint16_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(m_materialTextureHandles[idx].magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }

 private:
  HashMap<const char *, MaterialHandle, hashString32> m_nameToHandle;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<MaterialData> m_materialTextureHandles;
};

}  // namespace SirEngine::dx12
