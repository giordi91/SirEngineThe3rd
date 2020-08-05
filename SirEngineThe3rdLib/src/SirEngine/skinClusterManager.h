#pragma once
#include "SirEngine/handle.h"
#include "memory/cpu/SparseMemoryPool.h"
#include <stdint.h>
#include <string>
#include <unordered_map>

namespace SirEngine {
struct SkinData {
  BufferHandle weightsBuffer;
  BufferHandle influencesBuffer;
  BufferHandle matricesBuffer;
  // used to look up the matrices we want to use for the render
  AnimationConfigHandle animHandle;
  uint32_t magicNumber;
};

class SkinClusterManager final {

public:
  SkinClusterManager() : m_skinPool(RESERVE_SIZE) {}
  virtual ~SkinClusterManager() = default;
  // here mostly for consistency of interface
  void init(){};
  SkinClusterManager(const SkinClusterManager &) = delete;
  SkinClusterManager &operator=(const SkinClusterManager &) = delete;

  SkinHandle loadSkinCluster(const char *path,
                             AnimationConfigHandle animHandle);
  inline const SkinData &getSkinData(const SkinHandle handle) const
  {
	  assertMagicNumber(handle);
	  const uint32_t idx = getIndexFromHandle(handle);
	  return m_skinPool.getConstRef(idx);
  }

  void uploadDirtyMatrices();

private:
  static inline uint32_t getIndexFromHandle(const SkinHandle h) {
    return h.handle & INDEX_MASK;
  }

  static inline uint16_t getMagicFromHandle(const SkinHandle h) {
    return static_cast<uint16_t>((h.handle & MAGIC_NUMBER_MASK) >> 16);
  }
  inline void assertMagicNumber(const SkinHandle handle) const {
	  const uint32_t magic = getMagicFromHandle(handle);
	  const uint32_t idx = getIndexFromHandle(handle);
    assert(m_skinPool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for skin data");
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