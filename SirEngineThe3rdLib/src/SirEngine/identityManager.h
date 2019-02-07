#pragma once

#include "SirEngine/log.h"
#include "SirEngine/memory/SparseMemoryPool.h"
#include <vector>

namespace SirEngine {

class IdentityManager final {
private:
public:
  IdentityManager() : m_identityPool(RESERVE_SIZE) {}
  ~IdentityManager();
  IdentityManager(const IdentityManager &) = delete;
  IdentityManager &operator=(const IdentityManager &) = delete;

private:
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  SparseMemoryPool<const char *> m_identityPool;
};

} // namespace SirEngine
