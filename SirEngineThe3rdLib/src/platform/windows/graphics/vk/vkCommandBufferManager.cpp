
#include "platform/windows/graphics/vk/vkCommandBufferManager.h"

#include "SirEngine/engineConfig.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/vk/vk.h"
#include "vkLoad.h"

namespace SirEngine::vk {
void VkCommandBufferManager::freeBuffer(CommandBufferHandle handle)
{
  assertVersion(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  auto &data = m_bufferPool[idx];
  vkFreeCommandBuffers(LOGICAL_DEVICE, data.pool, 1, &data.buffer);
  vkDestroyCommandPool(LOGICAL_DEVICE, data.pool, nullptr);
}

bool VkCommandBufferManager::createCommandPool(
    const VkDevice logicalDevice, const VkCommandPoolCreateFlags parameters,
    const uint32_t queueFamily, VkCommandPool &commandPool) {
  VkCommandPoolCreateInfo commandPoolCreateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, parameters,
      queueFamily};

  const VkResult result = vkCreateCommandPool(
      logicalDevice, &commandPoolCreateInfo, nullptr, &commandPool);
  if (VK_SUCCESS != result) {
    SE_CORE_ERROR("Could not allocate command pool.");
    return false;
  }
  return true;
}
bool VkCommandBufferManager::allocateCommandBuffer(
    const VkDevice logicalDevice, const VkCommandPool commandPool,
    const VkCommandBufferLevel level, VkCommandBuffer &commandBuffer) {
  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, commandPool,
      level, 1};

  const VkResult result = vkAllocateCommandBuffers(
      logicalDevice, &commandBufferAllocateInfo, &commandBuffer);
  if (VK_SUCCESS != result) {
    SE_CORE_ERROR("Could not allocate command buffer.");
    return false;
  }
  return true;
}

CommandBufferHandle VkCommandBufferManager::createBuffer(
    const COMMAND_BUFFER_ALLOCATION_FLAGS flags, const char *name) {
  uint32_t index;
  VkCommandBufferData &data = m_bufferPool.getFreeMemoryData(index);

  VkCommandPoolCreateFlagBits params =
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  // TODO here might need some love for async compute or presentation queue if
  // different from normal queue
  createCommandPool(vk::LOGICAL_DEVICE, params,
                    vk::ADAPTER.m_graphicsQueueFamilyIndex, data.pool);
  if (name != nullptr) {
    SET_DEBUG_NAME(data.pool, VK_OBJECT_TYPE_COMMAND_POOL,
                   frameConcatenation("commandPool", name));
  }

  // allocate buffer
  allocateCommandBuffer(vk::LOGICAL_DEVICE, data.pool,
                        VK_COMMAND_BUFFER_LEVEL_PRIMARY, data.buffer);
  if (name != nullptr) {
    SET_DEBUG_NAME(data.buffer, VK_OBJECT_TYPE_COMMAND_BUFFER,
                   frameConcatenation("commandBuffer", name));
  }
  data.version = m_versionCounter++;

  return {data.version << 16 | index};
}

void VkCommandBufferManager::executeBuffer(const CommandBufferHandle handle) {

  assertVersion(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  auto &data = m_bufferPool[idx];
  VK_CHECK(vkEndCommandBuffer(data.buffer));
  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &data.buffer;
  VK_CHECK(vkQueueSubmit(vk::GRAPHICS_QUEUE, 1, &submitInfo, nullptr));
}

void VkCommandBufferManager::resetBufferHandle(CommandBufferHandle handle) {
  assertVersion(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  auto &data = m_bufferPool[idx];
  vkResetCommandPool(LOGICAL_DEVICE, data.pool, 0);
  beginCommandBufferRecordingOperation(
      data.buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
}

void VkCommandBufferManager::flush(CommandBufferHandle handle)
{
    vkDeviceWaitIdle(vk::LOGICAL_DEVICE);
}

void VkCommandBufferManager::executeFlushAndReset(CommandBufferHandle handle) {
    executeBuffer(handle);
    flush(handle);
    resetBufferHandle(handle);
}

}  // namespace SirEngine::vk
