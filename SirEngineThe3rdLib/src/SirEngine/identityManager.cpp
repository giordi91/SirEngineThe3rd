#include "SirEngine/identityManager.h"

namespace SirEngine {
IdentityManager::~IdentityManager() {
  assert(m_identityPool.assertEverythingDealloc());
}

} // namespace SirEngine
