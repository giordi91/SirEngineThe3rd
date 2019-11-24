#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "SirEngine/fileUtils.h"

namespace SirEngine::vk {
void VkPSOManager::init() {}

void VkPSOManager::loadRawPSO(const char *path)
{
	assert(fileExists(path));
}

} // namespace SirEngine::vk
