#pragma once

#include <vulkan/vulkan_core.h>

#include "SirEngine/graphics/commandBufferManager.h"
#include "SirEngine/memory/cpu/SparseMemoryPool.h"

namespace SirEngine::vk {

class VkCommandBufferManager final : public CommandBufferManager {
  static constexpr uint32_t RESERVE_SIZE = 50;
  static constexpr uint32_t MAX_IN_FLIGHT_BUFFERS = 50;

 public:
  struct VkCommandBufferData {
    VkCommandPool pool;
    VkCommandBuffer buffer;
    COMMAND_BUFFER_ALLOCATION_FLAGS flags;
    uint32_t version;
  };

 public:
  VkCommandBufferManager()
      : CommandBufferManager(), m_bufferPool(RESERVE_SIZE) {}
  ~VkCommandBufferManager() override = default;

  VkCommandBufferManager(const VkCommandBufferManager &) = delete;
  VkCommandBufferManager &operator=(const VkCommandBufferManager &) = delete;

  void initialize() override {}
  void cleanup() override {}

  CommandBufferHandle createBuffer(COMMAND_BUFFER_ALLOCATION_FLAGS flags,
                                   const char *name = nullptr) override;
  void executeBuffer(CommandBufferHandle handle) override;
  void resetBufferHandle(CommandBufferHandle handle) override;
  void flush(CommandBufferHandle handle) override;
  void executeFlushAndReset(CommandBufferHandle handle) override;
  void freeBuffer(CommandBufferHandle handle) override;

  // TODO temporary
  [[nodiscard]] const VkCommandBufferData &getData(
      const CommandBufferHandle handle) const {
    assertVersion(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    return m_bufferPool.getConstRef(idx);
  }

 private:
  static bool createCommandPool(const VkDevice logicalDevice,
                                const VkCommandPoolCreateFlags parameters,
                                const uint32_t queueFamily,
                                VkCommandPool &commandPool);
  static bool allocateCommandBuffer(const VkDevice logicalDevice,
                                    const VkCommandPool commandPool,
                                    const VkCommandBufferLevel level,
                                    VkCommandBuffer &commandBuffer);

  inline void assertVersion(const CommandBufferHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    auto const &ref = m_bufferPool.getConstRef(idx);
    assert(ref.version == magic && "invalid magic handle for command buffer");
  }

  static bool beginCommandBufferRecordingOperation(
      const VkCommandBuffer commandBuffer,
      const VkCommandBufferUsageFlags usage,
      VkCommandBufferInheritanceInfo *secondaryCommandBufferInfo);

 private:
  SparseMemoryPool<VkCommandBufferData> m_bufferPool;
  uint32_t m_versionCounter = 1;
};

}  // namespace SirEngine::vk
