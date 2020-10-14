#pragma once

#include "SirEngine/graphics/commandBufferManager.h"
#include "SirEngine/memory/cpu/SparseMemoryPool.h"

namespace SirEngine::vk {

class VkCommandBufferManager final : public CommandBufferManager {
 public:
  VkCommandBufferManager()
      : CommandBufferManager(), m_bufferPool(RESERVE_SIZE) {}
  ~VkCommandBufferManager() override = default;

  VkCommandBufferManager(const VkCommandBufferManager &) = delete;
  VkCommandBufferManager &operator=(const VkCommandBufferManager &) = delete;

  void initialize() override{}
  void cleanup() override{}

  CommandBufferHandle createBuffer() override;
  void executeBuffer(BufferHandle handle) override;
  void resetBufferHandle(BufferHandle handle) override;
  void executeFlushAndReset(BufferHandle handle) override;
  void releaseBuffer(BufferHandle handle) override;
 private:
  struct VkBufferData {};

 private:
  static constexpr uint32_t RESERVE_SIZE = 50;
  SparseMemoryPool<VkBufferData> m_bufferPool;

  // default texture
  TextureHandle m_whiteTexture{};
};

}  // namespace SirEngine::vk
