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

struct MaterialCPU final {
  dx12::ConstantBufferHandle cbHandle;
  dx12::TextureHandle albedo;
  dx12::TextureHandle normal;
};

class MaterialManager final {

public:
  MaterialManager() : m_idxPool(RESERVE_SIZE) {
    m_materialsCPU.resize(RESERVE_SIZE);
    m_materials.resize(RESERVE_SIZE);
  };
  ~MaterialManager() = default;
  MaterialManager(const MaterialManager &) = delete;
  MaterialManager &operator=(const MaterialManager &) = delete;

  void initialize();
  MaterialHandle loadMaterial(const char *path);

private:
  std::unordered_map<std::string, MaterialHandle> m_nameToHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;

  SparseMemoryPool<uint32_t> m_idxPool;
  std::vector<MaterialCPU> m_materialsCPU;
  std::vector<Material> m_materials;
};

} // namespace SirEngine
