#pragma once

#include <cassert>

#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/handle.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/memory/cpu/stringHashMap.h"

namespace SirEngine {

struct ShaderBind {
  RSHandle rs;
  PSOHandle pso;
};

struct MaterialDataHandles {
  TextureHandle albedo;
  TextureHandle normal;
  TextureHandle metallic;
  TextureHandle roughness;
  TextureHandle thickness;
  TextureHandle separateAlpha;
  TextureHandle ao;
  TextureHandle height;
  ConstantBufferHandle cbHandle;
  SkinHandle skinHandle;
};

enum class STENCIL_REF { CLEAR = 0, SSSSS = 1 };
#define INVALID_QUEUE_TYPE_FLAGS 0xFFFFFFFF

// TODO revise this struct
struct Material final {
  float reflective;
  float roughnessMult;
  float metallicMult;
  float padding;
};

enum class MATERIAL_RESOURCE_TYPE { TEXTURE, CONSTANT_BUFFER, BUFFER };
enum class MATERIAL_RESOURCE_FLAGS {
  NONE = 0,
  READ_ONLY = 1,
  MESH_VERTICES = 2,
  MESH_NORMALS = 4,
  MESH_UVS = 8,
  MESH_TANGENTS = 16,
};

struct MaterialMeshBinding {
  int binding;
  MESH_ATTRIBUTE_FLAGS flags;
};
struct MaterialResource {
  MATERIAL_RESOURCE_TYPE type;
  GRAPHIC_RESOURCE_VISIBILITY visibility;
  const char *name;
  MATERIAL_RESOURCE_FLAGS flags;
  uint16_t set;
  uint16_t binding;
};
struct MaterialMetadata {
  MaterialResource *objectResources;
  MaterialResource *frameResources;
  MaterialResource *passResources;
  uint32_t objectResourceCount;
  uint32_t frameResourceCount;
  uint32_t passResourceCount;
  MaterialMeshBinding meshBinding;
};

static constexpr uint32_t QUEUE_COUNT = 5;
struct MaterialRuntime final {
  ShaderBind shaderQueueTypeFlags2[QUEUE_COUNT] = {};
  SkinHandle skinHandle;
  MeshHandle meshHandle;
  DescriptorHandle descriptorHandles[5]{{}, {}, {}, {}, {}};
  uint8_t useStaticSamplers[5]{1, 1, 1, 1, 1};
  BindingTableHandle bindingHandle[QUEUE_COUNT]{};
};

struct MaterialData {
  MaterialDataHandles handles;
  Material m_material;
  MaterialRuntime m_materialRuntime;
  // PSOHandle m_psoHandle;
  // RSHandle m_rsHandle;
  // DescriptorHandle m_descriptorHandle;
  uint32_t magicNumber;
  const char *name = nullptr;
};

MaterialMetadata SIR_ENGINE_API extractMetadata(const char *psoPath);
MaterialMetadata SIR_ENGINE_API loadMetadata(const char *psoPath,
                                             GRAPHIC_API api);
class MaterialManager {
 public:
  static constexpr uint32_t QUEUE_COUNT = 5;
  MaterialManager()
      : m_nameToHandle(RESERVE_SIZE), m_materialTextureHandles(RESERVE_SIZE) {}
  ~MaterialManager() = default;
  void inititialize() {}
  void cleanup(){};
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag, const MaterialHandle handle);
  void bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                    const MaterialRuntime &materialRuntime);

  static inline uint64_t getQueueFlags(const uint64_t flags) {
    constexpr uint64_t mask = (1ll << 16) - 1ll;
    const uint64_t queueFlags = flags & mask;
    return queueFlags;
  }

  static inline bool isQueueType(const uint64_t flags,
                                 const SHADER_QUEUE_FLAGS queue) {
    const uint64_t queueFlags = getQueueFlags(flags);
    return (queueFlags & static_cast<uint64_t>(queue)) > 0;
  }
  // TODO see note on the dx12 material manager for this function
  ShaderBind bindRSandPSO(const uint64_t shaderFlags,
                          const MaterialHandle handle) const;
  MaterialManager(const MaterialManager &) = delete;
  MaterialManager &operator=(const MaterialManager &) = delete;

  void parseQueue(uint32_t *queues);
  MaterialHandle loadMaterial(const char *path, const MeshHandle meshHandle,
                              const SkinHandle skinHandle);

  // vk methods
  [[nodiscard]] const MaterialRuntime &getMaterialRuntime(
      const MaterialHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_materialTextureHandles.getConstRef(index).m_materialRuntime;
  }
  const MaterialData &getMaterialData(const MaterialHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_materialTextureHandles.getConstRef(index);
  }

  // called only on shutdown, main goal is to release GPU resources to
  // ease up the validation layer
  void releaseAllMaterialsAndRelatedResources();

 private:
  inline void assertMagicNumber(const MaterialHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(m_materialTextureHandles.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for material data");
  }

 public:
  void bindTexture(const MaterialHandle matHandle,
                   const TextureHandle texHandle,
                   const uint32_t descriptorIndex, const uint32_t bindingIndex,
                   SHADER_QUEUE_FLAGS queue, const bool isCubeMap);
  void bindBuffer(MaterialHandle matHandle, BufferHandle bufferHandle,
                  uint32_t bindingIndex, SHADER_QUEUE_FLAGS queue);

  void bindMaterial(MaterialHandle handle, SHADER_QUEUE_FLAGS queue);
  void free(MaterialHandle handle);

  void bindMesh(const MaterialHandle handle, const MeshHandle texHandle,
                const uint32_t descriptorIndex, const uint32_t bindingIndex,
                const uint32_t meshBindFlags, SHADER_QUEUE_FLAGS queue);

  void bindConstantBuffer(MaterialHandle handle,
                          ConstantBufferHandle bufferHandle,
                          const uint32_t descriptorIndex,
                          const uint32_t bindingIndex,
                          SHADER_QUEUE_FLAGS queue);

 private:
  struct PreliminaryMaterialParse {
    Material mat;
    MaterialDataHandles handles;
    uint32_t shaderQueueTypeFlags[QUEUE_COUNT] = {
        INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
        INVALID_QUEUE_TYPE_FLAGS, INVALID_QUEUE_TYPE_FLAGS,
        INVALID_QUEUE_TYPE_FLAGS};
    const char *shaderQueueTypeFlagsStr[QUEUE_COUNT] = {
        nullptr, nullptr, nullptr, nullptr, nullptr};
    // defines whether or not we expect the material to change, if not we can
    // save some resource allocations
    bool isStatic = false;
  };
  PreliminaryMaterialParse MaterialManager::parseMaterial(
      const char *path, const MeshHandle, const SkinHandle skinHandle);

 private:
  HashMap<const char *, MaterialHandle, hashString32> m_nameToHandle;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<MaterialData> m_materialTextureHandles;
  graphics::BindingDescription descriptions[16];
};

}  // namespace SirEngine
