#pragma once

#include <cassert>

#include "SirEngine/handle.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/memory/stringHashMap.h"
#include "platform/windows/graphics/vk/volk.h"
namespace SirEngine::vk {

struct VkMaterialRuntime final {
  VkDescriptorBufferInfo cbVirtualAddress;
  VkDescriptorImageInfo albedo;
  VkDescriptorImageInfo normal;
  VkDescriptorImageInfo metallic;
  VkDescriptorImageInfo roughness;
  VkDescriptorImageInfo thickness;
  VkDescriptorImageInfo separateAlpha;
  VkDescriptorImageInfo heightMap;
  VkDescriptorImageInfo ao;
  uint32_t shaderQueueTypeFlags[MaterialManager::QUEUE_COUNT] = {
      INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
      INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
      INVALID_QUEUE_TYPE_FLAGS};
  SkinHandle skinHandle;
  MeshHandle meshHandle;
  DescriptorHandle descriptorHandles[5]{{}, {}, {}, {}, {}};
  VkPipelineLayout layouts[5]{nullptr, nullptr, nullptr, nullptr};
  uint8_t useStaticSamplers[5]{1, 1, 1, 1, 1};
};

struct VkMaterialData {
  MaterialDataHandles handles;
  Material m_material;
  VkMaterialRuntime m_materialRuntime;
  // PSOHandle m_psoHandle;
  // RSHandle m_rsHandle;
  // DescriptorHandle m_descriptorHandle;
  uint32_t magicNumber;
  const char *name = nullptr;
};

class VkMaterialManager final : public MaterialManager {
 public:
  VkMaterialManager()
      : MaterialManager(RESERVE_SIZE),
        m_nameToHandle(RESERVE_SIZE),
        m_materialTextureHandles(RESERVE_SIZE){};
  ~VkMaterialManager() = default;
  void inititialize() override{};
  void cleanup() override{};
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag, const MaterialHandle handle,
                    VkCommandBuffer commandList);
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                    const VkMaterialRuntime &materialRuntime,
                    VkCommandBuffer commandList);
  void updateMaterial(SHADER_QUEUE_FLAGS queueFlag, MaterialHandle handle,
                      VkCommandBuffer commandList);

  void bindRSandPSO(uint32_t shaderFlags, VkCommandBuffer commandList);
  VkMaterialManager(const VkMaterialManager &) = delete;
  VkMaterialManager &operator=(const VkMaterialManager &) = delete;

  MaterialHandle loadMaterial(const char *path, const MeshHandle meshHandle,
                              const SkinHandle skinHandle);

  // vk methods
  const VkMaterialRuntime &getMaterialRuntime(const MaterialHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_materialTextureHandles.getConstRef(index).m_materialRuntime;
  }
  const VkMaterialData &getMaterialData(const MaterialHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_materialTextureHandles.getConstRef(index);
  }

  // called only on shutdown, main goal is to release GPU resources to
  // ease up the validation layer
  void releaseAllMaterialsAndRelatedResources();

 private:
  inline void assertMagicNumber(const MaterialHandle handle) {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(m_materialTextureHandles[idx].magicNumber == magic &&
           "invalid magic handle for material data");
  }

 public:
  MaterialHandle allocateMaterial(
      const char *name, ALLOCATE_MATERIAL_FLAGS flags,
      const char *materialsPerQueue[QUEUE_COUNT]) override;

  void bindTexture(const MaterialHandle matHandle,
                   const TextureHandle texHandle,
                   const uint32_t descriptorIndex, const uint32_t bindingIndex,
                   SHADER_QUEUE_FLAGS queue, const bool isCubeMap) override;
  void bindBuffer(MaterialHandle matHandle, BufferHandle bufferHandle,
                  uint32_t bindingIndex, SHADER_QUEUE_FLAGS queue) override;
  ;

  void bindMaterial(MaterialHandle handle, SHADER_QUEUE_FLAGS queue) override;
  void free(MaterialHandle handle) override;

  void bindMesh(const MaterialHandle handle, const MeshHandle texHandle,
                const uint32_t descriptorIndex, const uint32_t bindingIndex,
                const uint32_t meshBindFlags,
                SHADER_QUEUE_FLAGS queue) override;

 private:
  HashMap<const char *, MaterialHandle, hashString32> m_nameToHandle;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<VkMaterialData> m_materialTextureHandles;
};

}  // namespace SirEngine::vk
