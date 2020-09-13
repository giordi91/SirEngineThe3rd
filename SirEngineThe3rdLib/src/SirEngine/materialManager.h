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

enum class STENCIL_REF { CLEAR = 0, SSSSS = 1 };
#define INVALID_QUEUE_TYPE_FLAGS 0xFFFFFFFF



static constexpr uint32_t QUEUE_COUNT = 5;
struct MaterialRuntime final {
  ShaderBind shaderQueueTypeFlags[QUEUE_COUNT] = {};
  SkinHandle skinHandle;
  MeshHandle meshHandle;
  BindingTableHandle bindingHandle[QUEUE_COUNT]{};
};

struct MaterialData {
  MaterialRuntime m_materialRuntime;
  uint32_t magicNumber;
  const char *name = nullptr;
};

class MaterialManager {
 public:
  static constexpr uint32_t QUEUE_COUNT = 5;
  MaterialManager()
      : m_nameToHandle(RESERVE_SIZE), m_materialTextureHandles(RESERVE_SIZE) {}
  ~MaterialManager() = default;
  MaterialManager(const MaterialManager &) = delete;
  MaterialManager &operator=(const MaterialManager &) = delete;

  void inititialize() {}
  void cleanup() { releaseAllMaterialsAndRelatedResources(); }

  void bindMaterial(MaterialHandle handle, SHADER_QUEUE_FLAGS queue);
  void free(MaterialHandle handle);

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

  MaterialHandle loadMaterial(const char *path, const MeshHandle meshHandle,
                              const SkinHandle skinHandle);

  [[nodiscard]] const MaterialRuntime &getMaterialRuntime(
      const MaterialHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_materialTextureHandles.getConstRef(index).m_materialRuntime;
  }

 private:
  // called only on shutdown, main goal is to release GPU resources to
  // ease up the validation layer
  void releaseAllMaterialsAndRelatedResources();

  inline void assertMagicNumber(const MaterialHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(m_materialTextureHandles.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for material data");
  }

 private:
  struct PreliminaryMaterialParse {
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
  graphics::BindingDescription m_descriptions[16];
};

}  // namespace SirEngine
