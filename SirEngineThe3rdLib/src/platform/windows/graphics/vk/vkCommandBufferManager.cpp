
#include "platform/windows/graphics/vk/vkCommandBufferManager.h"

#include "platform/windows/graphics/vk/vk.h"

namespace SirEngine::vk {
CommandBufferHandle VkCommandBufferManager::createBuffer() {
  /*
VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
cmdBufAllocateInfo.commandPool = vk::;
cmdBufAllocateInfo.level = level;
cmdBufAllocateInfo.commandBufferCount = 1;

VkCommandBuffer cmdBuffer;
VK_CHECK(vkAllocateCommandBuffers(LOGICAL_DEVICE, &cmdBufAllocateInfo,
                                  &cmdBuffer));

// If requested, also start recording for the new command buffer
if (begin) {
  VkCommandBufferBeginInfo cmdBufInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
}

return cmdBuffer;
*/
  return {};
}
}  // namespace SirEngine::vk
