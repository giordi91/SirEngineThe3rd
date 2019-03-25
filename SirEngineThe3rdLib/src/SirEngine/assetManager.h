#pragma once
#include "identityManager.h"
#include <vector>
#include "SirEngine/handle.h"
#include "materialManager.h"
#include "platform/windows/graphics/dx12/meshManager.h"
#include <DirectXMath.h>

namespace SirEngine {

struct AssetDataHandle {
  union {
    uint32_t handle;
  };
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

  inline const dx12::MeshRuntime *getMeshRuntimes(uint32_t &count) const {
    count = allocIndex;
    return m_meshRuntime.data();
  };

  // assets
  IdentityHandle loadAsset(const char *path);
  void loadScene(const char *path);
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

  // handles
  inline AssetDataHandle getStaticDataHandle(const AssetDataType type) const {
    AssetDataHandle h;
	h.handle = 1 << 31 | type;
    return h;
  }
  inline uint32_t getEntryPointFromHandle(const AssetDataHandle h)
  {
	  return h.handle & (~(1<<31));
  }
  inline uint32_t getStaticDataFromHandle(const AssetDataHandle h)
  {
	  return ((h.handle >>31)& 1u);
  }

  inline const dx12::MeshRuntime *
  getRuntimeMeshesFromHandle(const AssetDataHandle h, uint32_t& count) {


	  uint32_t entryPoint = getEntryPointFromHandle(h);
    assert(entryPoint == AssetDataType::MESHES);
	uint32_t staticData = getStaticDataFromHandle(h);
    if (staticData) {
      count = allocIndex;
      return m_meshRuntime.data();
    } else {
      assert(0 && "dynamic data not supported yet");
      return nullptr;
    }
  }
  inline const MaterialRuntime*
  getRuntimeMaterialsFromHandle(const AssetDataHandle h, uint32_t& count) {

	  uint32_t entryPoint = getEntryPointFromHandle(h);
    assert(entryPoint == AssetDataType::MATERIALS);
	uint32_t staticData = getStaticDataFromHandle(h);
    if (staticData) {
      count = allocIndex;
      return m_materialRuntime.data();
    } else {
      assert(0 && "dynamic data not supported yet");
      return nullptr;
    }
  }
  // inline AssetDataBlob getRuntimeDataFromHandle(const AssetDataHandle h) {
  //  if (h.staticData) {
  //    switch (h.entryPoint) {
  //    case (AssetDataType::MATRICES): {
  //      return AssetDataBlob{m_matrixRuntime.data(), m_matrixRuntime.size()};
  //    }
  //    case (AssetDataType::MESHES): {
  //      return AssetDataBlob{m_meshRuntime.data(), m_meshRuntime.size()};
  //    }
  //    case (AssetDataType::MATERIALS): {
  //      return AssetDataBlob{m_materialRuntime.data(),
  //                           m_materialRuntime.size()};
  //    }
  //    default: {
  //      return AssetDataBlob{nullptr, 0};
  //    }
  //    }

  //  } else {
  //    assert(0 && "dynamic data not supported yet");
  //  }
  //}

private:
  // constants for allocations and handles
  static const uint32_t DATA_SIZE_ALLOC = 200;

  // allocations
  std::vector<AssetHandles> m_assetHandles;
  std::vector<IdentityHandle> m_idHandles;
  std::unordered_map<uint32_t, uint32_t> m_identityToIndex;
  //TODO alloc index is not a stable index in case of deletion of an asset need
  //to fix
  uint32_t allocIndex = 0;

  std::vector<DirectX::XMMATRIX> m_matrixRuntime;
  std::vector<dx12::MeshRuntime> m_meshRuntime;
  std::vector<MaterialRuntime> m_materialRuntime;
}; // namespace SirEngine
} // namespace SirEngine
