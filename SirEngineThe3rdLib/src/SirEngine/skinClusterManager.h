#pragma once
#include "SirEngine/handle.h"
#include "memory/SparseMemoryPool.h"
#include <stdint.h>
#include <string>
#include <unordered_map>

namespace SirEngine {
class SkinClusterManager final {
  struct SkinData {
    BufferHandle weightsBuffer;
    BufferHandle influencesBuffer;
    // used to look up the matrices we want to use for the render
    AnimationConfigHandle animHandle;
	uint32_t magicNumber;
  };


public:
  SkinClusterManager():m_skinPool(RESERVE_SIZE){}
  virtual ~SkinClusterManager() = default;
  //here mostly for consistency of interface
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
  // TODO can we do anything about it?
  std::unordered_map<std::string, SkinHandle> m_nameToHandle;
  SparseMemoryPool<SkinData> m_skinPool;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};

} // namespace SirEngine