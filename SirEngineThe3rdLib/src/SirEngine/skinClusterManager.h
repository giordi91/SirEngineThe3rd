#pragma once
#include "SirEngine/handle.h"
#include <stdint.h>

namespace SirEngine {
class SkinClusterManager final {
  struct SkinData {
    BufferHandle weightsBuffer;
    BufferHandle influencesBuffer;
    // used to look up the matrices we want to use for the render
    AnimationConfigHandle animHandle;
  };

  // BufferHandle AssetManager::loadSkin(const std::string &skinPath) {
  //  if (skinPath.empty()) {
  //    return BufferHandle{0};
  //  }
  //
  //  const BufferHandle cachedHandle =
  //      dx12::BUFFER_MANAGER->getBufferFromName(skinPath);
  //  if (cachedHandle.isHandleValid()) {
  //    return cachedHandle;
  //  }
  //}

public:
  SkinClusterManager() = default;
  virtual ~SkinClusterManager() = default;
  void init(){};
  SkinClusterManager(const SkinClusterManager &) = delete;
  SkinClusterManager &operator=(const SkinClusterManager &) = delete;

  SkinHandle loadSkinCluster(const char *path,
                             AnimationConfigHandle animHandle);

private:
  static inline uint32_t getIndexFromHandle(const SkinHandle h) {
    return h.handle & INDEX_MASK;
  }

  static inline uint16_t getMagicFromHandle(const SkinHandle h) {
    return static_cast<uint16_t>((h.handle & MAGIC_NUMBER_MASK) >> 16);
  }

private:
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};

} // namespace SirEngine