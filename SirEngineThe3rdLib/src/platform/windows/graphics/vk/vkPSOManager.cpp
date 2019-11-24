#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "SirEngine/fileUtils.h"

namespace SirEngine::vk {
void VkPSOManager::init() {}

void VkPSOManager::loadRawPSO(const char *path) {
  assert(fileExists(path));
  // ShHandle compiler = ShConstructCompiler(FindLanguage("stdin"), options);
}

} // namespace SirEngine::vk
