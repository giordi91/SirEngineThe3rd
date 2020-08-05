#pragma once
#include "SirEngine/handle.h"
#include "memory/cpu/resizableVector.h"
#include "memory/cpu/sparseMemoryPool.h"

namespace SirEngine {

class AssetManager final {
  struct AssetData {
    AssetDataHandle *m_subAssets = nullptr;
    uint32_t magicNumber;
    const char *name;
  };

public:
  AssetManager() : m_assetDatabase(RESERVE_SIZE){};
  void initialize(){};
  void cleanup();
  ~AssetManager() = default;
  AssetManager(const AssetManager &) = delete;
  AssetManager &operator=(const AssetManager &) = delete;

  // assets
  AssetDataHandle loadAsset(const char *path);
  AssetDataHandle loadScene(const char *path);

private:
  SparseMemoryPool<AssetData> m_assetDatabase;
  static constexpr uint32_t RESERVE_SIZE = 400;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  // TODO temporary list of stuff to clean up, until we will have a proper
  // asset definition of sort
  ResizableVector<MaterialHandle> m_materialsToFree;
};
} // namespace SirEngine
