#pragma once

#include <cassert>

#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/handle.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/memory/cpu/stringHashMap.h"

namespace SirEngine::vk {

struct VkMaterialRuntime final {
  ShaderBind shaderQueueTypeFlags2[MaterialManager::QUEUE_COUNT] = {};
  SkinHandle skinHandle;
  MeshHandle meshHandle;
  DescriptorHandle descriptorHandles[5]{{}, {}, {}, {}, {}};
  uint8_t useStaticSamplers[5]{1, 1, 1, 1, 1};
  BindingTableHandle bindingHandle[MaterialManager::QUEUE_COUNT]{};
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
      : MaterialManager(),
        m_nameToHandle(RESERVE_SIZE),
        m_materialTextureHandles(RESERVE_SIZE) {}
  ~VkMaterialManager() override = default;
  void inititialize() override {}
  void cleanup() override{};
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag, const MaterialHandle handle);
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                    const VkMaterialRuntime &materialRuntime);

  // TODO see note on the dx12 material manager for this function
  ShaderBind bindRSandPSO(uint64_t shaderFlags,
                          const VkMaterialRuntime &runtime) const;
  VkMaterialManager(const VkMaterialManager &) = delete;
  VkMaterialManager &operator=(const VkMaterialManager &) = delete;

  void parseQueue(uint32_t *queues);
  MaterialHandle loadMaterial(const char *path, const MeshHandle meshHandle,
                              const SkinHandle skinHandle) override;

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
  void bindTexture(const MaterialHandle matHandle,
                   const TextureHandle texHandle,
                   const uint32_t descriptorIndex, const uint32_t bindingIndex,
                   SHADER_QUEUE_FLAGS queue, const bool isCubeMap) override;
  void bindBuffer(MaterialHandle matHandle, BufferHandle bufferHandle,
                  uint32_t bindingIndex, SHADER_QUEUE_FLAGS queue) override;

  void bindMaterial(MaterialHandle handle, SHADER_QUEUE_FLAGS queue) override;
  void free(MaterialHandle handle) override;

  void bindMesh(const MaterialHandle handle, const MeshHandle texHandle,
                const uint32_t descriptorIndex, const uint32_t bindingIndex,
                const uint32_t meshBindFlags,
                SHADER_QUEUE_FLAGS queue) override;

  void bindConstantBuffer(MaterialHandle handle,
                          ConstantBufferHandle bufferHandle,
                          const uint32_t descriptorIndex,
                          const uint32_t bindingIndex,
                          SHADER_QUEUE_FLAGS queue) override;

 private:
  HashMap<const char *, MaterialHandle, hashString32> m_nameToHandle;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<VkMaterialData> m_materialTextureHandles;
  graphics::BindingDescription descriptions[16];
};

}  // namespace SirEngine::vk
