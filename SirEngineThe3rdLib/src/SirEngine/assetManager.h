#pragma once
#include "SirEngine/handle.h"

namespace SirEngine {

// TODO THIS IS POSSIBLY THE MOST HORRIBLE PART OF THE ENGINE IT NEEDS REWORK
// COMPLETELY

enum ASSET_DATA_TYPE { MATRICES = 1, MESHES = 2, MATERIALS = 3 };

class AssetManager final {
public:
  AssetManager() = default;
  void initialize(){};
  ~AssetManager() = default;
  AssetManager(const AssetManager &) = delete;
  AssetManager &operator=(const AssetManager &) = delete;

  // assets
  AssetDataHandle loadAsset(const char *path);
  void loadScene(const char *path);

private:
    //TODO will need to save those asset handles
};
} // namespace SirEngine
