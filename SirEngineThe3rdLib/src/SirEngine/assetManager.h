#pragma once
#include "SirEngine/handle.h"
#include "identityManager.h"
#include "materialManager.h"
#include "platform/windows/graphics/dx12/meshManager.h"
#include <DirectXMath.h>
#include <vector>

namespace SirEngine {

struct AssetDataHandle {
  union {
    uint32_t handle;
  };
};

struct Renderable {
  DirectX::XMMATRIX m_matrixRuntime;
  dx12::MeshRuntime m_meshRuntime;
  MaterialRuntime m_materialRuntime;
};

enum AssetDataType { MATRICES = 1, MESHES = 2, MATERIALS = 3 };

struct AssetDataBlob {
  void *data;
  uint32_t count;
};

class AssetManager final {
public:
  AssetManager();
  bool initialize();
  ~AssetManager() = default;
  AssetManager(const AssetManager &) = delete;
  AssetManager &operator=(const AssetManager &) = delete;


  // assets
  IdentityHandle loadAsset(const char *path);
  void loadScene(const char *path);
  AssetHandles getAssetHandle(const IdentityHandle handle) const {
    const auto found = m_identityToIndex.find(handle.handle);
    if (found != m_identityToIndex.end()) {
      return m_assetHandles[found->second];
    }
    assert(0 && "could not find requested asset handle from identity handle");
    return AssetHandles{};
  }

  inline const std::unordered_map<uint32_t, std::vector<Renderable>> &
  getRenderables() const {
    return m_renderables;
  }

  // handles
  inline AssetDataHandle getStaticDataHandle(const AssetDataType type) const {
    AssetDataHandle h{};
    h.handle = (1 << 31) | type;
	return h;
  }
  inline uint32_t getEntryPointFromHandle(const AssetDataHandle h) {
    return h.handle & (~(1 << 31));
  }
  inline uint32_t getStaticDataFromHandle(const AssetDataHandle h) {
    return ((h.handle >> 31) & 1u);
  }

private:
  // constants for allocations and handles
  static const uint32_t DATA_SIZE_ALLOC = 200;

  // allocations
  std::vector<AssetHandles> m_assetHandles;
  std::vector<IdentityHandle> m_idHandles;
  std::unordered_map<uint32_t, uint32_t> m_identityToIndex;
  // TODO alloc index is not a stable index in case of deletion of an asset need
  // to fix
  uint32_t allocIndex = 0;

  // new method
  std::unordered_map<uint32_t, std::vector<Renderable>> m_renderables;
}; // namespace SirEngine
} // namespace SirEngine
