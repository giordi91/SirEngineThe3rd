#pragma once

#include "SirEngine/log.h"
// TODO texture manager should not be a dx12 one
#include "platform/windows/graphics/dx12/constantBufferManager.h"
#include "platform/windows/graphics/dx12/textureManager.h"

namespace SirEngine {

struct MaterialHandle final {
  uint32_t handle;
};

struct Material final {
  float kDR;
  float kDG;
  float kDB;
  float kAR;
  float kAG;
  float kAB;
  float kSR;
  float kSG;
  float kSB;
};

enum SHADER_PASS_FLAGS { FORWARD = 1 };

struct MaterialCPU final {
  dx12::ConstantBufferHandle cbHandle;
  dx12::TextureHandle albedo;
  dx12::TextureHandle normal;
  uint32_t shaderFlags = 0;
};

class MaterialManager final {

public:
  MaterialManager() : m_idxPool(RESERVE_SIZE) {
    m_materialsCPU.resize(RESERVE_SIZE);
    m_materials.resize(RESERVE_SIZE), m_materialsMagic.resize(RESERVE_SIZE);
  };
  ~MaterialManager() = default;
  MaterialManager(const MaterialManager &) = delete;
  MaterialManager &operator=(const MaterialManager &) = delete;

  void initialize();
  inline uint32_t getIndexFromHandel(MaterialHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint16_t getMagicFromHandel(MaterialHandle h) const {
    return static_cast<uint16_t>((h.handle & MAGIC_NUMBER_MASK) >> 16);
  }

  inline void assertMagicNumber(MaterialHandle handle) {
    uint16_t magic = getMagicFromHandel(handle);
    uint32_t idx = getIndexFromHandel(handle);
    assert(m_materialsMagic[idx] == magic &&
           "invalid magic handle for constant buffer");
  }
  MaterialHandle loadMaterial(const char *path);
  const MaterialCPU &getMaterialCpu(const MaterialHandle handle) {
    assertMagicNumber(handle);
    uint32_t idx = getIndexFromHandel(handle);
    return m_materialsCPU[idx];
  }

private:
  std::unordered_map<std::string, MaterialHandle> m_nameToHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<uint32_t> m_idxPool;
  std::vector<MaterialCPU> m_materialsCPU;
  std::vector<Material> m_materials;
  std::vector<uint16_t> m_materialsMagic;
};

} // namespace SirEngine