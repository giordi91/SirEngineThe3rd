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

  inline const MaterialHandle *getMaterials(uint32_t &count) {
	  count = allocIndex;
    return m_materials.data();
  };

  IdentityHandle loadAsset(const char *path);

private:
  std::vector<IdentityHandle> m_handles;
  std::unordered_map<uint32_t, uint32_t> m_identityToIndex;
  std::vector<dx12::MeshHandle> m_meshes;
  std::vector<MaterialHandle> m_materials;
  static const uint32_t DATA_SIZE_ALLOC = 200;
  uint32_t allocIndex = 0;
};

} // namespace SirEngine
