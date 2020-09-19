#pragma once

#include <cassert>

#include "SirEngine/graphics/bindingTableManager.h"
#include "SirEngine/handle.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/memory/cpu/stringHashMap.h"
#include "engineMath.h"
#include "graphics/materialMetadata.h"

namespace SirEngine {

struct ShaderBind {
  RSHandle rs;
  PSOHandle pso;
};

enum class STENCIL_REF { CLEAR = 0, SSSSS = 1 };
#define INVALID_QUEUE_TYPE_FLAGS 0xFFFFFFFF

struct MaterialSourceSubBinding {
  char name[32];
  NUMERICAL_DATA_TYPE type;
  int sizeInByte;
  char value[64];
};
// This is a material binding coming from a file
struct MaterialSourceBinding {
  const char *type = nullptr;
  const char *bindingName = nullptr;
  const char *resourcePath = nullptr;
  MaterialSourceSubBinding *subBinding = nullptr;
  uint32_t subBindingCount = 0;
};

static constexpr uint32_t QUEUE_COUNT = 5;

struct MaterialData {
  MaterialSourceBinding *materialBinding;
  uint32_t materialBindingCount : 16;
  ShaderBind shaderBindPerQueue[QUEUE_COUNT] = {};
  BindingTableHandle bindingHandle[QUEUE_COUNT]{};
  uint32_t magicNumber : 16;
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
  void cleanup();

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
  ShaderBind bindRSandPSO(const uint64_t shaderFlags,
                          const MaterialHandle handle) const;

  void buildBindingTableDefinitionFromMetadta(
      const graphics::MaterialMetadata *meta);
  MaterialHandle loadMaterial(const char *path);

  [[nodiscard]] const MaterialData &getMaterialData(
      const MaterialHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_materialTextureHandles.getConstRef(index);
  }
  inline PSOHandle getmaterialPSO(const MaterialHandle handle,
                                  SHADER_QUEUE_FLAGS queue) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const auto &data = m_materialTextureHandles.getConstRef(index);
    int queueIdx = getFirstBitSet(static_cast<uint32_t>(queue));
    assert(queueIdx < QUEUE_COUNT);
    return data.shaderBindPerQueue[queueIdx].pso;
  };

 private:
  inline void assertMagicNumber(const MaterialHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(m_materialTextureHandles.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for material data");
  }

  struct PreliminaryMaterialParse {
    const char *shaderQueueTypeFlagsStr[QUEUE_COUNT] = {
        nullptr, nullptr, nullptr, nullptr, nullptr};
    MaterialSourceBinding *sourceBindings;
    uint32_t sourceBindingsCount;
    // defines whether or not we expect the material to change, if not we can
    // save some resource allocations
    bool isStatic = false;
  };

  PreliminaryMaterialParse parseMaterial(const char *path);

  HashMap<const char *, MaterialHandle, hashString32> m_nameToHandle;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<MaterialData> m_materialTextureHandles;
  graphics::BindingDescription m_descriptions[16];
};

}  // namespace SirEngine
