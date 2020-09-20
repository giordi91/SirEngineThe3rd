#include "platform/windows/graphics/vk/vkTextureManager.h"

#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "gli/gli.hpp"
#include "vk.h"
#include "vkBufferManager.h"

namespace SirEngine::vk {

static const std::string TEXTURE_CUBE_KEY = "cube";
static const std::string TEXTURE_FORMAT_KEY = "format";
static const std::string TEXTURE_GAMMA_KEY = "gamma";
static const std::string TEXTURE_HAS_MIPS_KEY = "hasMips";
static const std::string TEXTURE_PATH_KEY = "path";
static const std::string DEFAULT_STRING = "";
static const int DEFAULT_INT = 0;
static const bool DEFAULT_BOOL = false;
static const char *WHITE_TEXTURE_PATH =
    "../data/processed/textures/white.texture";

static std::unordered_map<RenderTargetFormat, VkFormat>
    RENDER_TARGET_FORMAT_TO_VK_FORMAT{
        {RenderTargetFormat::RGBA32, VK_FORMAT_R8G8B8A8_UNORM},
        {RenderTargetFormat::R16G16B16A16_FLOAT, VK_FORMAT_R16G16B16A16_SFLOAT},
        {RenderTargetFormat::BC1_UNORM, VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
        {RenderTargetFormat::DEPTH_F32_S8, VK_FORMAT_D32_SFLOAT_S8_UINT}};

// TODO This value come from the texture compiler which spits out dxgi formats,
// should fix this at one point?
static std::unordered_map<int, VkFormat> BC_INT_FORMAT_TO_VK_FORMAT{
    {44, VK_FORMAT_B8G8R8A8_UNORM},       {50, VK_FORMAT_B8G8R8A8_SRGB},
    {71, VK_FORMAT_BC1_RGBA_UNORM_BLOCK}, {72, VK_FORMAT_BC1_RGBA_SRGB_BLOCK},
    {77, VK_FORMAT_BC3_UNORM_BLOCK},      {78, VK_FORMAT_BC3_SRGB_BLOCK},
};

inline VkFormat convertRTFormatToVKFormat(const RenderTargetFormat format) {
  auto found = RENDER_TARGET_FORMAT_TO_VK_FORMAT.find(format);
  if (found != RENDER_TARGET_FORMAT_TO_VK_FORMAT.end()) {
    return found->second;
  }
  assert(0 && "Could not convert render target format to VkFormat");
  return VK_FORMAT_UNDEFINED;
}
inline VkFormat convertIntFormatToVKFormat(const int format) {
  auto found = BC_INT_FORMAT_TO_VK_FORMAT.find(format);
  if (found != BC_INT_FORMAT_TO_VK_FORMAT.end()) {
    return found->second;
  }
  assert(0 && "Could not convert int format to VkFormat");
  return VK_FORMAT_UNDEFINED;
}

VkTextureManager::~VkTextureManager() {
  // assert(m_texturePool.assertEverythingDealloc());
}
static void setImageLayout(
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
      // Only valid as initial layout for linear images, preserves memory
      // contents Make sure host writes have been finished
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

bool VkTextureManager::loadTextureFromFile(const char *name, VkFormat format,
                                           VkDevice device,
                                           VkTexture2D &outTexture,
                                           VkImageUsageFlags imageUsageFlags,
                                           VkImageLayout imageLayout,
                                           bool isCube) const {
  std::ifstream f(name);
  assert(!f.fail());

  gli::texture tex = gli::load(name);
  if (!isCube) {
    gli::texture2d tex2D(tex);
    assert(!tex2D.empty());

    const std::string textureName = getFileName(name);

    outTexture.width = static_cast<uint32_t>(tex2D[0].extent().x);
    outTexture.height = static_cast<uint32_t>(tex2D[0].extent().y);
    outTexture.mipLevels = static_cast<uint32_t>(tex2D.levels());

    // Get device properties for the requested texture format
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(PHYSICAL_DEVICE, format,
                                        &formatProperties);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(PHYSICAL_DEVICE, &memoryProperties);

    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    VkMemoryRequirements memReqs;

    // create a command buffer separated to execute this stuff
    VkCommandBuffer buffer =
        createCommandBuffer(CURRENT_FRAME_COMMAND->m_commandAllocator,
                            VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    SET_DEBUG_NAME(buffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "tempTextureBuffer");

    // Create a host-visible staging buffer that contains the raw image data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    VkBufferCreateInfo bufferCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferCreateInfo.size = tex2D.size();
    // This buffer is used as a transfer source for the buffer copy
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(
        vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));
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
    VK_CHECK(
        vkCreateImage(device, &imageCreateInfo, nullptr, &outTexture.image));
    SET_DEBUG_NAME(outTexture.image, VK_OBJECT_TYPE_IMAGE,
                   (textureName + "Image").c_str())

    vkGetImageMemoryRequirements(device, outTexture.image, &memReqs);

    memAllocInfo.allocationSize = memReqs.size;

    memAllocInfo.memoryTypeIndex =
        selectMemoryType(memoryProperties, memReqs.memoryTypeBits,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK(vkAllocateMemory(device, &memAllocInfo, nullptr,
                              &outTexture.deviceMemory));
    SET_DEBUG_NAME(outTexture.deviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                   (textureName + "DeviceMemory").c_str())
    VK_CHECK(vkBindImageMemory(device, outTexture.image,
                               outTexture.deviceMemory, 0));

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
    setImageLayout(buffer, outTexture.image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout,
                   subresourceRange);

    // TODO fix hardcoded
    flushCommandBuffer(CURRENT_FRAME_COMMAND->m_commandAllocator, buffer,
                       GRAPHICS_QUEUE, true);
    // Clean up staging resources
    vkFreeMemory(device, stagingMemory, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);

    // Create image view
    // Textures are not directly accessed by the shaders and
    // are abstracted by image views containing additional
    // information and sub resource ranges
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                                 VK_COMPONENT_SWIZZLE_B,
                                 VK_COMPONENT_SWIZZLE_A};
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

    outTexture.srv.sampler = 0;
    outTexture.srv.imageView = outTexture.view;
    outTexture.srv.imageLayout = imageLayout;

    return true;
  } else {
    // load cube texture
    gli::texture_cube tex2D(tex);
    assert(!tex2D.empty());

    gli::gl GL(gli::gl::PROFILE_GL33);
    gli::gl::format const translatedFormat =
        GL.translate(tex2D.format(), tex2D.swizzles());

    const std::string textureName = getFileName(name);

    outTexture.width = static_cast<uint32_t>(tex2D[0].extent().x);
    outTexture.height = static_cast<uint32_t>(tex2D[0].extent().y);
    outTexture.mipLevels = static_cast<uint32_t>(tex2D.levels());

    // Get device properties for the requested texture format
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(PHYSICAL_DEVICE, format,
                                        &formatProperties);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(PHYSICAL_DEVICE, &memoryProperties);

    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    VkMemoryRequirements memReqs;

    // create a command buffer separated to execute this stuff
    VkCommandBuffer buffer =
        createCommandBuffer(CURRENT_FRAME_COMMAND->m_commandAllocator,
                            VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    SET_DEBUG_NAME(buffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "tempTextureBuffer");

    // Create a host-visible staging buffer that contains the raw image data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    VkBufferCreateInfo bufferCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferCreateInfo.size = tex2D.size();
    // This buffer is used as a transfer source for the buffer copy
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(
        vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));
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

    // Create optimal tiled target image
    VkImageCreateInfo imageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;

    // imageCreateInfo.format = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
    imageCreateInfo.mipLevels = outTexture.mipLevels;
    imageCreateInfo.arrayLayers = 6;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = {outTexture.width, outTexture.height, 1};
    imageCreateInfo.usage = imageUsageFlags;
    // This flag is required for cube map images
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    // Ensure that the TRANSFER_DST bit is set for staging
    if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
      imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    VK_CHECK(
        vkCreateImage(device, &imageCreateInfo, nullptr, &outTexture.image));
    SET_DEBUG_NAME(outTexture.image, VK_OBJECT_TYPE_IMAGE,
                   (textureName + "Image").c_str())

    vkGetImageMemoryRequirements(device, outTexture.image, &memReqs);

    memAllocInfo.allocationSize = memReqs.size;

    memAllocInfo.memoryTypeIndex =
        selectMemoryType(memoryProperties, memReqs.memoryTypeBits,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK(vkAllocateMemory(device, &memAllocInfo, nullptr,
                              &outTexture.deviceMemory));
    SET_DEBUG_NAME(outTexture.deviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                   (textureName + "DeviceMemory").c_str())
    VK_CHECK(vkBindImageMemory(device, outTexture.image,
                               outTexture.deviceMemory, 0));

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = outTexture.mipLevels;
    subresourceRange.layerCount = 6;

    // Image barrier for optimal image (target)
    // Optimal image will be used as destination for the copy
    setImageLayout(buffer, outTexture.image, VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    // Setup buffer copy regions for each face including all of it's miplevels
    std::vector<VkBufferImageCopy> bufferCopyRegions;
    uint32_t offset = 0;

    for (uint32_t face = 0; face < 6; face++) {
      for (uint32_t level = 0; level < outTexture.mipLevels; level++) {
        // Calculate offset into staging buffer for the current mip level and
        // face
        const auto &currentTexture2D = tex2D[face];
        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = level;
        bufferCopyRegion.imageSubresource.baseArrayLayer = face;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width =
            static_cast<uint32_t>(currentTexture2D[level].extent().x);
        bufferCopyRegion.imageExtent.height =
            static_cast<uint32_t>(currentTexture2D[level].extent().y);
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = offset;
        bufferCopyRegions.push_back(bufferCopyRegion);

        offset += static_cast<uint32_t>(currentTexture2D[level].size());
      }
    }

    // Copy mip levels from staging buffer
    vkCmdCopyBufferToImage(buffer, stagingBuffer, outTexture.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(bufferCopyRegions.size()),
                           bufferCopyRegions.data());

    // Change texture image layout to shader read after all mip levels have been
    // copied
    outTexture.imageLayout = imageLayout;
    setImageLayout(buffer, outTexture.image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout,
                   subresourceRange);

    // TODO fix hardcoded
    flushCommandBuffer(CURRENT_FRAME_COMMAND->m_commandAllocator, buffer,
                       GRAPHICS_QUEUE, true);
    // Clean up staging resources
    vkFreeMemory(device, stagingMemory, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);

    // Create image view
    // Textures are not directly accessed by the shaders and
    // are abstracted by image views containing additional
    // information and sub resource ranges
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewCreateInfo.format = format;
    viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                                 VK_COMPONENT_SWIZZLE_B,
                                 VK_COMPONENT_SWIZZLE_A};
    viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6};
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
    outTexture.srv.sampler = 0;
    outTexture.srv.imageView = outTexture.view;
    outTexture.srv.imageLayout = imageLayout;

    return true;
  }
}

// TODO review clean this up a bit
TextureHandle VkTextureManager::loadTexture(const char *path,
                                            const bool)  // cube map
{
  const bool res = fileExists(path);
  assert(res);

  const auto jobj = getJsonObj(path);
  // TODO we need to store this info
  const bool isCube = getValueIfInJson(jobj, TEXTURE_CUBE_KEY, DEFAULT_BOOL);
  const bool isGamma = getValueIfInJson(jobj, TEXTURE_GAMMA_KEY, DEFAULT_BOOL);
  const bool hasMips =
      getValueIfInJson(jobj, TEXTURE_HAS_MIPS_KEY, DEFAULT_BOOL);
  const std::string texturePath =
      getValueIfInJson(jobj, TEXTURE_PATH_KEY, DEFAULT_STRING);
  const int formatInt = getValueIfInJson(jobj, TEXTURE_FORMAT_KEY, DEFAULT_INT);
  const VkFormat format = convertIntFormatToVKFormat(formatInt);

  const std::string name = getFileName(texturePath);

  TextureHandle handle{};
  if (!m_nameToHandle.containsKey(name.c_str())) {
    // const auto found = m_nameToHandle.find(name);
    // if (found == m_nameToHandle.end()) {
    const std::string extension = getFileExtension(texturePath);

    uint32_t index;
    VkTexture2D &data = m_texturePool.getFreeMemoryData(index);
    uint32_t usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;
    VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    loadTextureFromFile(texturePath.c_str(), format, vk::LOGICAL_DEVICE, data,
                        usageFlags, layout, isCube);

    // data is now loaded need to create handle etc
    handle = {(MAGIC_NUMBER_COUNTER << 16) | index};

    data.magicNumber = MAGIC_NUMBER_COUNTER;
    data.format = format;
    data.imageLayout = layout;
    data.name = persistentString(name.c_str());

    ++MAGIC_NUMBER_COUNTER;

    m_nameToHandle.insert(name.c_str(), handle);

    return handle;
  }
  SE_CORE_INFO("Texture already loaded, returning handle: {0}", name);
  return handle;
}

void VkTextureManager::free(const TextureHandle handle) {
  // by default if a texture is not present will get the white handle
  // to keep the asset destruction streamlined, we allow to pass
  // in the white texture, but ignore it
  if (handle.handle == m_whiteTexture.handle) {
    return;
  }
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  VkTexture2D &data = m_texturePool[index];

  // releasing the texture data and objects;
  vkDestroyImage(vk::LOGICAL_DEVICE, data.image, nullptr);
  vkDestroyImageView(vk::LOGICAL_DEVICE, data.view, nullptr);
  vkFreeMemory(vk::LOGICAL_DEVICE, data.deviceMemory, nullptr);
  // invalidating magic number
  if (m_nameToHandle.containsKey(data.name)) {
    m_nameToHandle.remove(data.name);
  }
  data = {};
  // adding the index to the free list
  m_texturePool.free(index);
}

TextureHandle VkTextureManager::allocateTexture(
    uint32_t width, uint32_t height, RenderTargetFormat format,
    const char *name, TEXTURE_ALLOCATION_FLAGS allocFlags,
    RESOURCE_STATE finalState) {
  // NOTE for now this is hardcoded, if necessary will be changed
  VkImageLayout imageLayout = fromStateToLayout(finalState);

  VkImageUsageFlags imageUsageFlags = 0;
  bool isRT = (allocFlags & TEXTURE_ALLOCATION_FLAG_BITS::RENDER_TARGET) > 0;
  bool isSrc = (allocFlags & TEXTURE_ALLOCATION_FLAG_BITS::COPY_SOURCE) > 0;
  bool isDest = (allocFlags & TEXTURE_ALLOCATION_FLAG_BITS::COPY_DEST) > 0;
  bool isStorage =
      (allocFlags & TEXTURE_ALLOCATION_FLAG_BITS::STORAGE_RESOURCE) > 0;
  bool isShader =
      (allocFlags & TEXTURE_ALLOCATION_FLAG_BITS::SHADER_RESOURCE) > 0;
  bool isDepth = (allocFlags & TEXTURE_ALLOCATION_FLAG_BITS::DEPTH_TEXTURE) > 0;
  imageUsageFlags |= isRT ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
  imageUsageFlags |= isSrc ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0;
  imageUsageFlags |= isDest ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
  imageUsageFlags |= isShader ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
  imageUsageFlags |= isDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0;
  imageUsageFlags |= isStorage ? VK_IMAGE_USAGE_STORAGE_BIT : 0;

  const std::string textureName = getFileName(name);

  uint32_t index;
  VkTexture2D &data = m_texturePool.getFreeMemoryData(index);
  data.width = width;
  data.height = height;
  data.mipLevels = 1;
  data.creationFlags = 0;
  data.isRenderTarget = 1;

  // need to convert genering render target format to vulkan render target
  // format
  VkFormat vkFormat = convertRTFormatToVKFormat(format);
  data.format = vkFormat;
  // Get device properties for the requested texture format
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(PHYSICAL_DEVICE, vkFormat,
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

  // Create optimal tiled target image
  VkImageCreateInfo imageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = vkFormat;
  imageCreateInfo.mipLevels = data.mipLevels;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.extent = {data.width, data.height, 1};
  imageCreateInfo.usage = imageUsageFlags;
  // Ensure that the TRANSFER_DST bit is set for staging
  if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
    imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }
  VK_CHECK(vkCreateImage(vk::LOGICAL_DEVICE, &imageCreateInfo, nullptr,
                         &data.image));
  SET_DEBUG_NAME(data.image, VK_OBJECT_TYPE_IMAGE,
                 frameConcatenation(textureName.c_str(), "Image"));

  vkGetImageMemoryRequirements(vk::LOGICAL_DEVICE, data.image, &memReqs);

  memAllocInfo.allocationSize = memReqs.size;

  memAllocInfo.memoryTypeIndex =
      selectMemoryType(memoryProperties, memReqs.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK(vkAllocateMemory(vk::LOGICAL_DEVICE, &memAllocInfo, nullptr,
                            &data.deviceMemory));
  SET_DEBUG_NAME(data.deviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                 frameConcatenation(textureName.c_str(), "Memory"));
  VK_CHECK(
      vkBindImageMemory(vk::LOGICAL_DEVICE, data.image, data.deviceMemory, 0));

  VkImageAspectFlags aspect =
      isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
              : VK_IMAGE_ASPECT_COLOR_BIT;
  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = aspect;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = data.mipLevels;
  subresourceRange.layerCount = 1;

  // Change texture image layout to shader read after
  data.imageLayout = imageLayout;
  setImageLayout(buffer, data.image, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout,
                 subresourceRange);

  flushCommandBuffer(CURRENT_FRAME_COMMAND->m_commandAllocator, buffer,
                     GRAPHICS_QUEUE, true);

  // Create image view
  // Textures are not directly accessed by the shaders and
  // are abstracted by image views containing additional
  // information and sub resource ranges
  VkImageViewCreateInfo viewCreateInfo = {};
  viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewCreateInfo.format = vkFormat;
  viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                               VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};

  viewCreateInfo.subresourceRange = {aspect, 0, 1, 0, 1};
  // Linear tiling usually won't support mip maps
  // Only set mip map count if optimal tiling is used
  viewCreateInfo.subresourceRange.levelCount = data.mipLevels;
  viewCreateInfo.image = data.image;
  VK_CHECK(vkCreateImageView(vk::LOGICAL_DEVICE, &viewCreateInfo, nullptr,
                             &data.view));
  SET_DEBUG_NAME(data.view, VK_OBJECT_TYPE_IMAGE_VIEW,
                 frameConcatenation(textureName.c_str(), "ImageView"))

  // Update descriptor image info member that can be used for setting up
  // descriptor sets
  data.srv.sampler = nullptr;
  data.srv.imageView = data.view;
  data.srv.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  data.name = persistentString(textureName.c_str());

  // build the handle
  data.magicNumber = MAGIC_NUMBER_COUNTER++;
  return {data.magicNumber << 16 | index};
}

void VkTextureManager::initialize() {
  m_whiteTexture = loadTexture(WHITE_TEXTURE_PATH, false);
}

void VkTextureManager::cleanup() {
  assertMagicNumber(m_whiteTexture);
  uint32_t index = getIndexFromHandle(m_whiteTexture);
  VkTexture2D &data = m_texturePool[index];

  // releasing the texture data and objects;
  vkDestroyImage(vk::LOGICAL_DEVICE, data.image, nullptr);
  vkDestroyImageView(vk::LOGICAL_DEVICE, data.view, nullptr);
  vkFreeMemory(vk::LOGICAL_DEVICE, data.deviceMemory, nullptr);
  data = {};
  m_texturePool.free(index);

  uint32_t count = m_nameToHandle.binCount();
  for (uint32_t i = 0; i < count; ++i) {
    if (m_nameToHandle.isBinUsed(i)) {
      const TextureHandle handle = m_nameToHandle.getValueAtBin(i);
      free(handle);
    }
  }
}
}  // namespace SirEngine::vk
