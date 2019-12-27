#pragma once
#include "SirEngine/handle.h"

namespace SirEngine {


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

};
} // namespace SirEngine
