
#include "platform/windows/graphics/vk/vkCommandBufferManager.h"

#include "SirEngine/log.h"
#include "platform/windows/graphics/vk/vk.h"
#include "vkLoad.h"

namespace SirEngine::vk {
// utility functions

struct SubmitCommandBuffersInfo {
  // wait semaphores
  VkSemaphore *waitSemaphore;
  VkPipelineStageFlags *waitSemaphoreStages;
  uint32_t waitSemaphoreCount;
  // signal semaphores
  VkSemaphore *signalSemaphores;
  uint32_t signalSemaphoreCount;
  // buffers
  VkCommandBuffer *buffers;
  uint32_t bufferCount;

  VkFence fence;
};

bool submitCommandBuffersToQueue(const VkQueue queue,
                                 const SubmitCommandBuffersInfo &info) {
  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             nullptr,
                             info.waitSemaphoreCount,
                             info.waitSemaphore,
                             info.waitSemaphoreStages,
                             info.bufferCount,
                             info.buffers,
                             info.signalSemaphoreCount,
                             info.signalSemaphores};

  const VkResult result = vkQueueSubmit(queue, 1, &submitInfo, info.fence);
  if (VK_SUCCESS != result) {
    SE_CORE_ERROR("Error occurred during command buffer submission.");
    return false;
  }
  return true;
}

bool createCommandPool(const VkDevice logicalDevice,
                       const VkCommandPoolCreateFlags parameters,
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
bool allocateCommandBuffer(const VkDevice logicalDevice,
                           const VkCommandPool commandPool,
                           const VkCommandBufferLevel level,
                           VkCommandBuffer &commandBuffer) {
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

bool beginCommandBufferRecordingOperation(
    const VkCommandBuffer commandBuffer, const VkCommandBufferUsageFlags usage,
    VkCommandBufferInheritanceInfo *secondaryCommandBufferInfo) {
  VkCommandBufferBeginInfo commandBufferBeginInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, usage,
      secondaryCommandBufferInfo};

  const VkResult result =
      vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
  if (VK_SUCCESS != result) {
    SE_CORE_ERROR("Could not begin command buffer recording operation.");
    return false;
  }
  return true;
}

// interface
void VkCommandBufferManager::freeBuffer(const CommandBufferHandle handle) {
  assertVersion(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  auto &data = m_bufferPool[idx];
  vkFreeCommandBuffers(LOGICAL_DEVICE, data.pool, 1, &data.buffer);
  vkDestroyCommandPool(LOGICAL_DEVICE, data.pool, nullptr);
  m_bufferPool.free(idx);
}

bool VkCommandBufferManager::executeBufferEndOfFrame(
    const CommandBufferHandle handle, const VkSemaphore acquireSemaphore,
    const VkSemaphore renderSemaphore, VkFence fence) {
  assertVersion(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  auto &data = m_bufferPool[idx];
  const VkResult result = vkEndCommandBuffer(data.buffer);
  assert(result == VK_SUCCESS);

  VkSemaphore acquireSem[1] = {acquireSemaphore};
  VkPipelineStageFlags acquireStage[1] = {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
  VkSemaphore sigSem[1] = {renderSemaphore};
  VkCommandBuffer buffers[1] = {data.buffer};
  SubmitCommandBuffersInfo submit{// wait semaphores
                                  acquireSem, acquireStage, 1,
                                  // signal semaphores
                                  sigSem, 1,
                                  // buffers
                                  buffers, 1, fence};

  if (!submitCommandBuffersToQueue(PRESENTATION_QUEUE, submit)) {
    return false;
  }
  return true;
}

CommandBufferHandle VkCommandBufferManager::createBuffer(
    const COMMAND_BUFFER_ALLOCATION_FLAGS, const char *name) {
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

void VkCommandBufferManager::executeFlushAndReset(
    const CommandBufferHandle handle) {
  executeBuffer(handle);
  globals::RENDERING_CONTEXT->flush();
  resetBufferHandle(handle);
}

}  // namespace SirEngine::vk
