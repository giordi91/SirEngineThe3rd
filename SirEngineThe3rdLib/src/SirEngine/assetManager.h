#pragma once
#include "SirEngine/handle.h"
#include "identityManager.h"
#include "materialManager.h"
#include "platform/windows/graphics/dx12/dx12MeshManager.h"
#include <vector>

namespace SirEngine {

// TODO THIS IS POSSIBLY THE MOST HORRIBLE PART OF THE ENGINE IT NEEDS REWORK
// COMPLETELY

struct AssetDataHandle {
  union {
    uint32_t handle;
  };
};

struct Renderable {
  glm::mat4 m_matrixRuntime;
  MeshHandle m_meshHandle;
  MaterialHandle m_materialHandle;
};

enum ASSET_DATA_TYPE { MATRICES = 1, MESHES = 2, MATERIALS = 3 };

class AssetManager final {
public:
  AssetManager() = default;
  bool init();
  ~AssetManager() = default;
  AssetManager(const AssetManager &) = delete;
  AssetManager &operator=(const AssetManager &) = delete;

  // assets
  IdentityHandle loadAsset(const char *path);
  void loadScene(const char *path);

  inline const std::unordered_map<uint32_t, std::vector<Renderable>> &
  getRenderables(const StreamHandle handle) const {
    const auto found = m_streamMapper.find(handle.handle);
    if (found != m_streamMapper.end()) {
      return found->second;
    }

    SE_CORE_ERROR("Could not find stream in asset manager");

    return m_streamMapper.find(m_masterHandle.handle)->second;
  }

  StreamHandle getMainStreamHandle() const { return m_masterHandle; }

private:
  // constants for allocations and handles
  static const uint32_t DATA_SIZE_ALLOC = 200;

  // allocations
  std::vector<IdentityHandle> m_idHandles;
  std::unordered_map<uint32_t, uint32_t> m_identityToIndex;
  // TODO alloc index is not a stable index in case of deletion of an asset need
  // to fix
  uint32_t allocIndex = 0;

  std::unordered_map<uint32_t, std::vector<Renderable>> *m_renderables;
  std::unordered_map<uint32_t,
                     std::unordered_map<uint32_t, std::vector<Renderable>>>
      m_streamMapper;
  StreamHandle m_masterHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
}; // namespace SirEngine
} // namespace SirEngine
