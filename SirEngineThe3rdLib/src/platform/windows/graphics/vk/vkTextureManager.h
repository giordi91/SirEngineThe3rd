#pragma once

#include "SirEngine/core.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/SparseMemoryPool.h"
#include "SirEngine/textureManager.h"
#include "platform/windows/graphics/vk/volk.h"

namespace SirEngine::vk {

class SIR_ENGINE_API VkTextureManager final : public TextureManager {
  struct VkTexture2DTemp {
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t magicNumber;
    VkImage image;
    VkDeviceMemory deviceMemory;
    VkImageLayout imageLayout;
    VkImageView view;
    VkDescriptorImageInfo descriptor{};
    VkFormat format;
    VkImageLayout layout;
  };

public:
  VkTextureManager() : m_texturePool(RESERVE_SIZE) {
    m_nameToHandle.reserve(RESERVE_SIZE);
  }
  virtual ~VkTextureManager();

  VkTextureManager(const VkTextureManager &) = delete;
  VkTextureManager &operator=(const VkTextureManager &) = delete;
  virtual TextureHandle loadTexture(const char *path,
                                    bool cubeMap = false) override;
  virtual void free(const TextureHandle handle) override;
  virtual TextureHandle allocateRenderTexture(uint32_t width, uint32_t height,
                                              RenderTargetFormat format,
                                              const char *name,
                                              bool allowWrite = false) override;
  virtual TextureHandle allocateTexture(uint32_t width, uint32_t height,
                                        RenderTargetFormat format,
                                        const char *name, bool mips,
                                        bool allowWrite = false) override;
  virtual void bindRenderTarget(TextureHandle handle,
                                TextureHandle depth) override;
  virtual void bindRenderTargetStencil(TextureHandle handle,
                                       TextureHandle depth);

  virtual void copyTexture(TextureHandle source,
                           TextureHandle destination) override;
  virtual void bindBackBuffer(bool bindBackBufferDepth) override;
  virtual void clearDepth(const TextureHandle depth,
                          float value = 1.0f) override;
  virtual void clearRT(const TextureHandle handle,
                       const float color[4]) override;

  void initialize() override;
  void cleanup() override;
  inline TextureHandle getWhiteTexture() const { return m_whiteTexture; }
  // vk methods
  void bindTexture(const TextureHandle &handle,
                   VkWriteDescriptorSet *writeDescriptorSets,
                   VkDescriptorSet descriptorSet, uint32_t bindSlot) {
    assertMagicNumber(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    const auto &data = m_texturePool.getConstRef(idx);

    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstSet = descriptorSet;
    writeDescriptorSets[0].dstBinding = bindSlot;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writeDescriptorSets[0].pImageInfo = &data.descriptor;
    writeDescriptorSets[0].descriptorCount = 1;
  };
  void bindTexture(const VkDescriptorImageInfo &info,
                   VkWriteDescriptorSet *writeDescriptorSets,
                   const VkDescriptorSet descriptorSet,
                   const uint32_t bindSlot) {

    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstSet = descriptorSet;
    writeDescriptorSets[0].dstBinding = bindSlot;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writeDescriptorSets[0].pImageInfo = &info;
    writeDescriptorSets[0].descriptorCount = 1;
  };

  inline VkDescriptorImageInfo
  getTextureDescriptor(const TextureHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    const auto &data = m_texturePool.getConstRef(idx);
    return data.descriptor;
  }

private:
  bool loadTextureFromFile(const char *name, VkFormat format, VkDevice device,
                           VkTexture2DTemp &outTexture,
                           VkImageUsageFlags imageUsageFlags,
                           VkImageLayout imageLayout,
                           bool isCube = false) const;
  inline void assertMagicNumber(const TextureHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(m_texturePool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }

private:
  std::unordered_map<std::string, TextureHandle> m_nameToHandle;
  SparseMemoryPool<VkTexture2DTemp> m_texturePool;

  // default texture
  TextureHandle m_whiteTexture;
};

} // namespace SirEngine::vk
