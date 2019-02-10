#include "SirEngine/IdentityManager.h"
#include "farmhash/farmhash.h"

namespace SirEngine {

void IdentityManager::initialize() { m_stack.initialize(STACK_SIZE_IN_BYTES); }

IdentityHandle IdentityManager::getHandleFromName(const char *name){

  uint32_t hashValue = util::Hash32(name, strlen(name));
  auto found = m_hashToName.find(hashValue);
  if (found != m_hashToName.end()) {
    return IdentityHandle{found->first};
  }
  else
  {
	  return createHandleFromName(name);
  }
}

IdentityHandle IdentityManager::createHandleFromName(const char *name) {
  uint32_t hashValue = util::Hash32(name, strlen(name));
  size_t sizeOfStr = strlen(name);

  auto found = m_hashToName.find(hashValue);
  if(found != m_hashToName.end())
  {assert(0 && "hash already generated");}
  // internalize the string
  char *data = static_cast<char *>(m_stack.allocate(sizeOfStr));
  memcpy(data, name, sizeOfStr);
  m_hashToName[hashValue] = data;
  return IdentityHandle{hashValue};
}

const char *IdentityManager::getNameFromhandle(IdentityHandle handle) const {
  auto found = m_hashToName.find(handle.handle);
  assert(found != m_hashToName.end());
  return found->second;
}
} // namespace SirEngine
