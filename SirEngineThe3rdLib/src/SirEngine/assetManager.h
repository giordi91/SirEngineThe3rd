#pragma once
#include "identityManager.h"
#include <vector>
// TODO declare a handle file so we can use handles without
#include "SirEngine/handle.h"
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

  inline const dx12::MeshRuntime *getMeshRuntimes(uint32_t &count) const {
    count = allocIndex;
    return m_meshRuntime.data();
  };

  // assets
  IdentityHandle loadAsset(const char *path);
  AssetHandles getAssetHandle(const IdentityHandle handle) const {
    auto found = m_identityToIndex.find(handle.handle);
    if (found != m_identityToIndex.end()) {
      return m_assetHandles[found->second];
    }
    assert(0 && "could not find requested asset handle from identity handle");
    return AssetHandles{};
  }

  // materials
  const MaterialRuntime *getMaterialsCPU(uint32_t &index) const {

    index = allocIndex;
    return m_materialRuntime.data();
  }

private:
  // constants for allocations and handles
  static const uint32_t DATA_SIZE_ALLOC = 200;

  // allocations
  std::vector<AssetHandles> m_assetHandles;
  std::vector<IdentityHandle> m_idHandles;
  std::unordered_map<uint32_t, uint32_t> m_identityToIndex;
  uint32_t allocIndex = 0;

  std::vector<dx12::MeshRuntime> m_meshRuntime;
  std::vector<MaterialRuntime> m_materialRuntime;
};
} // namespace SirEngine
