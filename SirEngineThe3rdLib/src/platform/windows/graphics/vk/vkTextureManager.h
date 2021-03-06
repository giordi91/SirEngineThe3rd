#pragma once

#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/cpu/SparseMemoryPool.h"
#include "SirEngine/textureManager.h"
#include "platform/windows/graphics/vk/volk.h"
#include "vkCommandBufferManager.h"

namespace SirEngine::vk {
struct VkTexture2D {
  const char *name = nullptr;
  VkImage image;
  VkDeviceMemory deviceMemory;
  VkImageView view;
  VkDescriptorImageInfo srv{};
  VkImageLayout imageLayout;
  VkFormat format;
  uint32_t width : 16;
  uint32_t height : 16;
  uint32_t mipLevels : 16;
  uint32_t magicNumber : 16;
  uint32_t isRenderTarget : 1;
  uint32_t creationFlags : 31;
};

class VkTextureManager final : public TextureManager {
 public:
  VkTextureManager() : TextureManager(), m_texturePool(RESERVE_SIZE) {}
  ~VkTextureManager() override;

  VkTextureManager(const VkTextureManager &) = delete;
  VkTextureManager &operator=(const VkTextureManager &) = delete;
  virtual TextureHandle loadTexture(const char *path,
                                    bool cubeMap = false) override;
  virtual void free(const TextureHandle handle) override;
  virtual TextureHandle allocateTexture(uint32_t width, uint32_t height,
                                        RenderTargetFormat format,
                                        const char *name,
                                        TEXTURE_ALLOCATION_FLAGS allocFlags,
                                        RESOURCE_STATE finalState) override;
  void transitionTexture(CommandBufferHandle commandBuffer,
                         TextureHandle texHandle, RESOURCE_STATE currState,
                         RESOURCE_STATE newState) override;

  void initialize() override;
  void cleanup() override;

  [[nodiscard]] TextureHandle getWhiteTexture() const override {
    return m_whiteTexture;
  }

  // vk methods
  const VkTexture2D &getTextureData(const TextureHandle &handle) const {
    assertMagicNumber(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    const auto &data = m_texturePool.getConstRef(idx);
    return data;
  };

  [[nodiscard]] VkFormat getTextureFormat(const TextureHandle &handle) const {
    assertMagicNumber(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    const auto &data = m_texturePool.getConstRef(idx);
    return data.format;
  };

  void bindTexture(const TextureHandle &handle,
                   VkWriteDescriptorSet *writeDescriptorSets,
                   const VkDescriptorSet descriptorSet, const uint32_t bindSlot,
                   const bool isCompute = false

  ) const {
    assertMagicNumber(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    const auto &data = m_texturePool.getConstRef(idx);

    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstSet = descriptorSet;
    writeDescriptorSets[0].dstBinding = bindSlot;
    writeDescriptorSets[0].descriptorType =
        isCompute ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                  : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

    writeDescriptorSets[0].pImageInfo = &data.srv;
    writeDescriptorSets[0].descriptorCount = 1;
  }
  void setImageLayout(
      VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout,
      VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange,
      VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VkPipelineStageFlags dstStageMask =
          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) const;

 private:
  bool loadTextureFromFile(const char *name, VkFormat format, VkDevice device,
                           VkTexture2D &outTexture,
                           VkImageUsageFlags imageUsageFlags,
                           VkImageLayout imageLayout,
                           bool isCube = false) const;
  inline void assertMagicNumber(const TextureHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    auto const &ref = m_texturePool.getConstRef(idx);
    assert(ref.magicNumber == magic && "invalid magic handle for texture ");
  }

 private:
  SparseMemoryPool<VkTexture2D> m_texturePool;

  // default texture
  TextureHandle m_whiteTexture;
  CommandBufferHandle m_workerBuffer;
};

}  // namespace SirEngine::vk
