#pragma once
#include "identityManager.h"
#include <vector>
// TODO declare a handle file so we can use handles without
// including managers itself
#include "materialManager.h"
#include "platform/windows/graphics/dx12/meshManager.h"

namespace SirEngine {

class AssetManager final {
public:
  AssetManager();
  bool initialize();
  ~AssetManager() = default;
  AssetManager(const AssetManager &) = delete;
  AssetManager &operator=(const AssetManager &) = delete;

  inline const dx12::MeshHandle *getMeshes(uint32_t &count) const {
    count = allocIndex;
    return m_meshes.data();
  };


  //assets
  IdentityHandle loadAsset(const char *path);

  // materials
  const MaterialCPU *getMaterialsCPU(uint32_t &index) const {

    index = allocIndex;
    return m_materialsCPU.data();
  }

private:
  // handle data mask
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;

  // allocations
  std::vector<IdentityHandle> m_idHandles;
  std::unordered_map<uint32_t, uint32_t> m_identityToIndex;
  static const uint32_t DATA_SIZE_ALLOC = 200;
  uint32_t allocIndex = 0;

  //meshes
  std::vector<dx12::MeshHandle> m_meshes;

  // materials
  Materials::MaterialsMemory m_materialMemory;
  std::vector<MaterialHandle> m_materialHandles;
  std::vector<MaterialCPU> m_materialsCPU;
  std::vector<Material> m_materials;
  std::vector<uint16_t> m_materialsMagic;

};
} // namespace SirEngine
