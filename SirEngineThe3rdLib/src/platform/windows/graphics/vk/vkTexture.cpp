#include "platform/windows/graphics/vk/vkTexture.h"
#include "gli/gli.hpp"
//#include "platform/windows/graphics/vk/VulkanFunctions.h"
#include "platform/windows/graphics/vk/vkMemory.h"

#include "SirEngine/fileUtils.h"
#include "platform/windows/graphics/vk/vk.h"
#include "volk.h"
#include <fstream>

namespace SirEngine::vk {

void setImageLayout(
    VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
  // Create an image barrier object
  VkImageMemoryBarrier imageMemoryBarrier = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  imageMemoryBarrier.oldLayout = oldImageLayout;
  imageMemoryBarrier.newLayout = newImageLayout;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange = subresourceRange;

  // Source layouts (old)
  // Source access mask controls actions that have to be finished on the old
  // layout before it will be transitioned to the new layout
  switch (oldImageLayout) {
  case VK_IMAGE_LAYOUT_UNDEFINED:
    // Image layout is undefined (or does not matter)
    // Only valid as initial layout
    // No flags required, listed only for completeness
    imageMemoryBarrier.srcAccessMask = 0;
    break;

  case VK_IMAGE_LAYOUT_PREINITIALIZED:
    // Image is preinitialized
    // Only valid as initial layout for linear images, preserves memory contents
    // Make sure host writes have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // Image is a color attachment
    // Make sure any writes to the color buffer have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // Image is a depth/stencil attachment
    // Make sure any writes to the depth/stencil buffer have been finished
    imageMemoryBarrier.srcAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // Image is a transfer source
    // Make sure any reads from the image have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // Image is a transfer destination
    // Make sure any writes to the image have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // Image is read by a shader
    // Make sure any shader reads from the image have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    // Other source layouts aren't handled (yet)
    break;
  }

  // Target layouts (new)
  // Destination access mask controls the dependency for the new image layout
  switch (newImageLayout) {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // Image will be used as a transfer destination
    // Make sure any writes to the image have been finished
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // Image will be used as a transfer source
    // Make sure any reads from the image have been finished
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;

  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // Image will be used as a color attachment
    // Make sure any writes to the color buffer have been finished
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // Image layout will be used as a depth/stencil attachment
    // Make sure any writes to depth/stencil buffer have been finished
    imageMemoryBarrier.dstAccessMask =
        imageMemoryBarrier.dstAccessMask |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // Image will be read in a shader (sampler, input attachment)
    // Make sure any writes to the image have been finished
    if (imageMemoryBarrier.srcAccessMask == 0) {
      imageMemoryBarrier.srcAccessMask =
          VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    // Other source layouts aren't handled (yet)
    break;
  }

  // Put barrier inside setup command buffer
  vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0,
                       nullptr, 1, &imageMemoryBarrier);
}

bool createRenderTarget(const char *name, VkFormat format, VkDevice device,
                        VkTexture2D &outTexture,
                        VkImageUsageFlags imageUsageFlags,
                        VkImageLayout imageLayout, uint32_t width,
                        uint32_t height) {

  const std::string textureName = getFileName(name);

  outTexture.width = width;
  outTexture.height = height;
  outTexture.mipLevels = 1;

  // Get device properties for the requested texture format
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(PHYSICAL_DEVICE, format,
                                      &formatProperties);

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(PHYSICAL_DEVICE, &memoryProperties);

  VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  VkMemoryRequirements memReqs;

  // create a command buffer separated to execute this stuff
  // TODO fix hardcoded index
  VkCommandBuffer buffer =
      createCommandBuffer(CURRENT_FRAME_COMMAND->m_commandAllocator,
                          VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
  SET_DEBUG_NAME(buffer, VK_OBJECT_TYPE_COMMAND_BUFFER,
                 frameConcatenation(name, "CommandBufferTemp"));

  /*
  // Create a host-visible staging buffer that contains the raw image data
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;

  VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferCreateInfo.size = tex2D.size();
  // This buffer is used as a transfer source for the buffer copy
  bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));
  SET_DEBUG_NAME(stagingBuffer, VK_OBJECT_TYPE_BUFFER,
                 (textureName + "Staging").c_str())

  // Get memory requirements for the staging buffer (alignment, memory type
  // bits)
  vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);

  memAllocInfo.allocationSize = memReqs.size;
  // Get memory type index for a host visible buffer
  memAllocInfo.memoryTypeIndex =
      selectMemoryType(memoryProperties, memReqs.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VK_CHECK(vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingMemory));
  SET_DEBUG_NAME(stagingMemory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                 (textureName + "Memory").c_str())
  VK_CHECK(vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0));

  // Copy texture data into staging buffer
  uint8_t *data;
  VK_CHECK(
      vkMapMemory(device, stagingMemory, 0, memReqs.size, 0, (void **)&data));
  memcpy(data, tex2D.data(), tex2D.size());
  vkUnmapMemory(device, stagingMemory);

  // Setup buffer copy regions for each mip level
  std::vector<VkBufferImageCopy> bufferCopyRegions;
  uint32_t offset = 0;

  for (uint32_t i = 0; i < outTexture.mipLevels; i++) {
    VkBufferImageCopy bufferCopyRegion = {};
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.mipLevel = i;
    bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageExtent.width =
        static_cast<uint32_t>(tex2D[i].extent().x);
    bufferCopyRegion.imageExtent.height =
        static_cast<uint32_t>(tex2D[i].extent().y);
    bufferCopyRegion.imageExtent.depth = 1;
    bufferCopyRegion.bufferOffset = offset;

    bufferCopyRegions.push_back(bufferCopyRegion);

    offset += static_cast<uint32_t>(tex2D[i].size());
  }
  */

  // Create optimal tiled target image
  VkImageCreateInfo imageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = format;
  imageCreateInfo.mipLevels = outTexture.mipLevels;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.extent = {outTexture.width, outTexture.height, 1};
  imageCreateInfo.usage = imageUsageFlags;
  // Ensure that the TRANSFER_DST bit is set for staging
  if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
    imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }
  VK_CHECK(vkCreateImage(device, &imageCreateInfo, nullptr, &outTexture.image));
  SET_DEBUG_NAME(outTexture.image, VK_OBJECT_TYPE_IMAGE,
                 frameConcatenation(textureName.c_str(), "Image"));

  vkGetImageMemoryRequirements(device, outTexture.image, &memReqs);

  memAllocInfo.allocationSize = memReqs.size;

  memAllocInfo.memoryTypeIndex =
      selectMemoryType(memoryProperties, memReqs.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK(vkAllocateMemory(device, &memAllocInfo, nullptr,
                            &outTexture.deviceMemory));
  SET_DEBUG_NAME(outTexture.deviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                 frameConcatenation(textureName.c_str(), "Memory"));
  VK_CHECK(
      vkBindImageMemory(device, outTexture.image, outTexture.deviceMemory, 0));

  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = outTexture.mipLevels;
  subresourceRange.layerCount = 1;

  // Change texture image layout to shader read after
  outTexture.imageLayout = imageLayout;
  setImageLayout(buffer, outTexture.image, VK_IMAGE_LAYOUT_UNDEFINED,
                 imageLayout, subresourceRange);

  flushCommandBuffer(CURRENT_FRAME_COMMAND->m_commandAllocator, buffer,
                     GRAPHICS_QUEUE, true);

  // Create image view
  // Textures are not directly accessed by the shaders and
  // are abstracted by image views containing additional
  // information and sub resource ranges
  VkImageViewCreateInfo viewCreateInfo = {};
  viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewCreateInfo.format = format;
  viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                               VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
  viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  // Linear tiling usually won't support mip maps
  // Only set mip map count if optimal tiling is used
  viewCreateInfo.subresourceRange.levelCount = outTexture.mipLevels;
  viewCreateInfo.image = outTexture.image;
  VK_CHECK(
      vkCreateImageView(device, &viewCreateInfo, nullptr, &outTexture.view));
  SET_DEBUG_NAME(outTexture.view, VK_OBJECT_TYPE_IMAGE_VIEW,
                 frameConcatenation(textureName.c_str(), "ImageView"))

  // Update descriptor image info member that can be used for setting up
  // descriptor sets
  /*
  updateDescriptor();
  */
  // outTexture.descriptor.sampler = outTexture.sampler;
  outTexture.descriptor.sampler = 0;
  outTexture.descriptor.imageView = outTexture.view;
  outTexture.descriptor.imageLayout = imageLayout;

  return true;
}

bool loadTextureFromFile(const char *name, VkFormat format, VkDevice device,
                         VkTexture2D &outTexture,
                         VkImageUsageFlags imageUsageFlags,
                         VkImageLayout imageLayout) {

  std::ifstream f(name);
  assert(!f.fail());

  gli::texture2d tex2D(gli::load(name));
  assert(!tex2D.empty());

  const std::string textureName = getFileName(name);

  outTexture.width = static_cast<uint32_t>(tex2D[0].extent().x);
  outTexture.height = static_cast<uint32_t>(tex2D[0].extent().y);
  outTexture.mipLevels = static_cast<uint32_t>(tex2D.levels());

  // Get device properites for the requested texture format
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(PHYSICAL_DEVICE, format,
                                      &formatProperties);

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(PHYSICAL_DEVICE, &memoryProperties);

  VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  VkMemoryRequirements memReqs;

  // create a command buffer separated to execute this stuff
  VkCommandBuffer buffer =
      createCommandBuffer(CURRENT_FRAME_COMMAND->m_commandAllocator,
                          VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
  SET_DEBUG_NAME(buffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "tempTextureBuffer");

  // Create a host-visible staging buffer that contains the raw image data
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;

  VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferCreateInfo.size = tex2D.size();
  // This buffer is used as a transfer source for the buffer copy
  bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));
  SET_DEBUG_NAME(stagingBuffer, VK_OBJECT_TYPE_BUFFER,
                 (textureName + "Staging").c_str())

  // Get memory requirements for the staging buffer (alignment, memory type
  // bits)
  vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);

  memAllocInfo.allocationSize = memReqs.size;
  // Get memory type index for a host visible buffer
  memAllocInfo.memoryTypeIndex =
      selectMemoryType(memoryProperties, memReqs.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VK_CHECK(vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingMemory));
  SET_DEBUG_NAME(stagingMemory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                 (textureName + "Memory").c_str())
  VK_CHECK(vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0));

  // Copy texture data into staging buffer
  uint8_t *data;
  VK_CHECK(
      vkMapMemory(device, stagingMemory, 0, memReqs.size, 0, (void **)&data));
  memcpy(data, tex2D.data(), tex2D.size());
  vkUnmapMemory(device, stagingMemory);

  // Setup buffer copy regions for each mip level
  std::vector<VkBufferImageCopy> bufferCopyRegions;
  uint32_t offset = 0;

  for (uint32_t i = 0; i < outTexture.mipLevels; i++) {
    VkBufferImageCopy bufferCopyRegion = {};
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.mipLevel = i;
    bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageExtent.width =
        static_cast<uint32_t>(tex2D[i].extent().x);
    bufferCopyRegion.imageExtent.height =
        static_cast<uint32_t>(tex2D[i].extent().y);
    bufferCopyRegion.imageExtent.depth = 1;
    bufferCopyRegion.bufferOffset = offset;

    bufferCopyRegions.push_back(bufferCopyRegion);

    offset += static_cast<uint32_t>(tex2D[i].size());
  }

  // Create optimal tiled target image
  VkImageCreateInfo imageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = format;
  imageCreateInfo.mipLevels = outTexture.mipLevels;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.extent = {outTexture.width, outTexture.height, 1};
  imageCreateInfo.usage = imageUsageFlags;
  // Ensure that the TRANSFER_DST bit is set for staging
  if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
    imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }
  VK_CHECK(vkCreateImage(device, &imageCreateInfo, nullptr, &outTexture.image));
  SET_DEBUG_NAME(outTexture.image, VK_OBJECT_TYPE_IMAGE,
                 (textureName + "Image").c_str())

  vkGetImageMemoryRequirements(device, outTexture.image, &memReqs);

  memAllocInfo.allocationSize = memReqs.size;

  memAllocInfo.memoryTypeIndex =
      selectMemoryType(memoryProperties, memReqs.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK(vkAllocateMemory(device, &memAllocInfo, nullptr,
                            &outTexture.deviceMemory));
  VK_CHECK(
      vkBindImageMemory(device, outTexture.image, outTexture.deviceMemory, 0));

  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = outTexture.mipLevels;
  subresourceRange.layerCount = 1;

  // Image barrier for optimal image (target)
  // Optimal image will be used as destination for the copy
  setImageLayout(buffer, outTexture.image, VK_IMAGE_LAYOUT_UNDEFINED,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

  // Copy mip levels from staging buffer
  vkCmdCopyBufferToImage(buffer, stagingBuffer, outTexture.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         static_cast<uint32_t>(bufferCopyRegions.size()),
                         bufferCopyRegions.data());

  // Change texture image layout to shader read after all mip levels have been
  // copied
  outTexture.imageLayout = imageLayout;
  setImageLayout(buffer, outTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 imageLayout, subresourceRange);

  // TODO fix hardcoded
  flushCommandBuffer(CURRENT_FRAME_COMMAND->m_commandAllocator, buffer,
                     GRAPHICS_QUEUE, true);
  // Clean up staging resources
  vkFreeMemory(device, stagingMemory, nullptr);
  vkDestroyBuffer(device, stagingBuffer, nullptr);

  // Create a defaultsampler
  VkSamplerCreateInfo samplerCreateInfo = {};
  samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.mipLodBias = 0.0f;
  samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
  samplerCreateInfo.minLod = 0.0f;
  // Max level-of-detail should match mip level count
  samplerCreateInfo.maxLod = static_cast<float>(outTexture.mipLevels);
  // Only enable anisotropic filtering if enabled on the devicec
  samplerCreateInfo.maxAnisotropy = 1.0f;
  // device->enabledFeatures.samplerAnisotropy
  //    ? device->properties.limits.maxSamplerAnisotropy
  //    : 1.0f;
  samplerCreateInfo.anisotropyEnable = false;
  // device->enabledFeatures.samplerAnisotropy;
  samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr,
                           &outTexture.sampler));

  // NOTE THIS NAME IS  TRASH needs to survive the whole life cycle
  auto samplerName = (textureName + "Sampler");
  SET_DEBUG_NAME(outTexture.sampler, VK_OBJECT_TYPE_SAMPLER,
                 samplerName.c_str())

  // Create image view
  // Textures are not directly accessed by the shaders and
  // are abstracted by image views containing additional
  // information and sub resource ranges
  VkImageViewCreateInfo viewCreateInfo = {};
  viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewCreateInfo.format = format;
  viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                               VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
  viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  // Linear tiling usually won't support mip maps
  // Only set mip map count if optimal tiling is used
  viewCreateInfo.subresourceRange.levelCount = outTexture.mipLevels;
  viewCreateInfo.image = outTexture.image;
  VK_CHECK(
      vkCreateImageView(device, &viewCreateInfo, nullptr, &outTexture.view));
  SET_DEBUG_NAME(outTexture.view, VK_OBJECT_TYPE_IMAGE_VIEW,
                 (textureName + "ImageView").c_str())

  // Update descriptor image info member that can be used for setting up
  // descriptor sets
  /*
  updateDescriptor();
  */
  // outTexture.descriptor.sampler = outTexture.sampler;
  outTexture.descriptor.sampler = 0;
  outTexture.descriptor.imageView = outTexture.view;
  outTexture.descriptor.imageLayout = imageLayout;

  outTexture.samplerOnly.sampler = outTexture.sampler;

  return true;
}

bool destroyTexture(const VkDevice device, const VkTexture2D &texture) {
  vkDestroyImage(device, texture.image, nullptr);
  vkDestroyImageView(device, texture.view, nullptr);
  vkDestroySampler(device, texture.sampler, nullptr);
  vkFreeMemory(device, texture.deviceMemory, nullptr);

  return true;
}

bool destroyFrameBuffer(const VkDevice device, const VkFramebuffer fb,
                        const VkTexture2D &texture) {
  vkDestroyImage(device, texture.image, nullptr);
  vkDestroyImageView(device, texture.view, nullptr);
  vkFreeMemory(device, texture.deviceMemory, nullptr);
  vkDestroyFramebuffer(device, fb, nullptr);
  return true;
}
} // namespace SirEngine::vk