#pragma once

#include "SirEngine/handle.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/memory/stringHashMap.h"
#include <cassert>

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
  uint32_t shaderQueueTypeFlags[4] = {
      INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
      INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS};
  SkinHandle skinHandle;
  MeshHandle meshHandle;
  DescriptorHandle descriptorHandles[4]{{}, {}, {}, {}};
};

struct MaterialData {
  MaterialDataHandles handles;
  uint32_t magicNumber;
  Material m_material;
  VkMaterialRuntime m_materialRuntime;
};

class VkMaterialManager final : public MaterialManager {
public:
  VkMaterialManager()
      : MaterialManager(RESERVE_SIZE), m_nameToHandle(RESERVE_SIZE),
        m_materialTextureHandles(RESERVE_SIZE){};
  ~VkMaterialManager() = default;
  void inititialize() override{};
  void cleanup() override{};
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag, const MaterialHandle handle,
                    VkCommandBuffer commandList);
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                    const VkMaterialRuntime &materialRuntime,
                    VkCommandBuffer commandList);
  void updateMaterial(SHADER_QUEUE_FLAGS queueFlag, MaterialHandle handle, VkCommandBuffer commandList);

  void bindRSandPSO(uint32_t shaderFlags, VkCommandBuffer commandList);
  VkMaterialManager(const VkMaterialManager &) = delete;
  VkMaterialManager &operator=(const VkMaterialManager &) = delete;

  MaterialHandle loadMaterial(const char *path, const MeshHandle meshHandle,
                              const SkinHandle skinHandle);

  const VkMaterialRuntime &getMaterialRuntime(const MaterialHandle handle) {
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
  void loadTypeFile(const char *path);

private:
  HashMap<const char *, MaterialHandle, hashString32> m_nameToHandle;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<MaterialData> m_materialTextureHandles;
};

} // namespace SirEngine::vk
