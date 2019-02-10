#pragma once

#include "SirEngine/log.h"
#include "SirEngine/memory/stackAllocator.h"

namespace SirEngine {

struct IdentityHandle {
  uint32_t handle;
};

class IdentityManager final {

public:
  IdentityManager() = default;
  ~IdentityManager() = default;
  IdentityManager(const IdentityManager &) = delete;
  IdentityManager &operator=(const IdentityManager &) = delete;

  void initialize();
  IdentityHandle getHandleFromName(const char *name);
  IdentityHandle createHandleFromName(const char *name);
  const char *getNameFromhandle(const IdentityHandle handle) const;

private:
  // 4 mb data
  static const uint32_t STACK_SIZE_IN_BYTES = static_cast<uint32_t>(4e6f);
  std::unordered_map<uint32_t, const char *> m_hashToName;
  StackAllocator m_stack;
};

} // namespace SirEngine
