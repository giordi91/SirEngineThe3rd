#include "platform/windows/graphics/vk/vkMemory.h"

#include <assert.h>

#include "platform/windows/graphics/vk/vk.h"

namespace SirEngine::vk {

VkCommandBuffer createCommandBuffer(const VkCommandPool pool,
                                    const VkCommandBufferLevel level,
                                    const bool begin) {
  VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
  cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdBufAllocateInfo.commandPool = pool;
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
}
void flushCommandBuffer(VkCommandPool pool, VkCommandBuffer commandBuffer,
                        const VkQueue queue, const bool free) {
  if (commandBuffer == VK_NULL_HANDLE) {
    return;
  }

  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  // Create fence to ensure that the command buffer has finished executing
  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = 0;
  VkFence fence;
  VK_CHECK(vkCreateFence(LOGICAL_DEVICE, &fenceInfo, nullptr, &fence));

  // Submit to the queue
  VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence));
  // Wait for the fence to signal that command buffer has finished executing

  VK_CHECK(vkWaitForFences(LOGICAL_DEVICE, 1, &fence, VK_TRUE, 100000000000));

  vkDestroyFence(LOGICAL_DEVICE, fence, nullptr);

  if (free) {
    vkFreeCommandBuffers(LOGICAL_DEVICE, pool, 1, &commandBuffer);
  }
}

}  // namespace SirEngine::vk
