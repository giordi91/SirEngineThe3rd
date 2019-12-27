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
        {RenderTargetFormat::BC1_UNORM, VK_FORMAT_BC1_RGBA_UNORM_BLOCK}};

// TODO This value come from the texture compiler which spits out dxgi formats,
// should fix this at one point?
static std::unordered_map<int, VkFormat> BC_INT_FORMAT_TO_VK_FORMAT{
    {71, VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
    {72, VK_FORMAT_BC1_RGBA_SRGB_BLOCK},
    {77, VK_FORMAT_BC3_UNORM_BLOCK},
    {78, VK_FORMAT_BC3_SRGB_BLOCK},
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

bool VkTextureManager::loadTextureFromFile(const char *name, VkFormat format,
                                           VkDevice device,
                                           VkTexture2DTemp &outTexture,
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

  // Get device properties for the requested texture format
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
  SET_DEBUG_NAME(outTexture.deviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                 (textureName + "DeviceMemory").c_str())
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

  return true;
}

TextureHandle VkTextureManager::loadTexture(const char *path,
                                            const bool cubeMap) {
  const bool res = fileExists(path);
  assert(res);

  const auto jobj = getJsonObj(path);
  const bool isCube = getValueIfInJson(jobj, TEXTURE_CUBE_KEY, DEFAULT_BOOL);
  const bool isGamma = getValueIfInJson(jobj, TEXTURE_GAMMA_KEY, DEFAULT_BOOL);
  const bool hasMips =
      getValueIfInJson(jobj, TEXTURE_HAS_MIPS_KEY, DEFAULT_BOOL);
  const std::string texturePath =
      getValueIfInJson(jobj, TEXTURE_PATH_KEY, DEFAULT_STRING);
  const int formatInt = getValueIfInJson(jobj, TEXTURE_FORMAT_KEY, DEFAULT_INT);
  const VkFormat format = convertIntFormatToVKFormat(formatInt);

  const std::string name = getFileName(texturePath);

  const auto found = m_nameToHandle.find(name);
  if (found == m_nameToHandle.end()) {
    const std::string extension = getFileExtension(texturePath);

    uint32_t index;
    VkTexture2DTemp &data = m_texturePool.getFreeMemoryData(index);
    uint32_t usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;
    VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    loadTextureFromFile(texturePath.c_str(), format, vk::LOGICAL_DEVICE, data,
                        usageFlags, layout);

    // data is now loaded need to create handle etc
    const TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

    data.magicNumber = MAGIC_NUMBER_COUNTER;
    data.format = format;
    data.layout = layout;

    ++MAGIC_NUMBER_COUNTER;

    m_nameToHandle[name] = handle;

    return handle;
  }
  SE_CORE_INFO("Texture already loaded, returning handle: {0}", name);
  return found->second;
}

void VkTextureManager::free(const TextureHandle handle) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  VkTexture2DTemp &data = m_texturePool[index];

  // releasing the texture data and objects;
  vkDestroyImage(vk::LOGICAL_DEVICE, data.image, nullptr);
  vkDestroyImageView(vk::LOGICAL_DEVICE, data.view, nullptr);
  vkFreeMemory(vk::LOGICAL_DEVICE, data.deviceMemory, nullptr);
  // invalidating magic number
  data.magicNumber = 0;
  // adding the index to the free list
  m_texturePool.free(index);
}

TextureHandle VkTextureManager::allocateRenderTexture(
    const uint32_t width, const uint32_t height,
    const RenderTargetFormat format, const char *name, bool allowWrite) {
  /*
// convert SirEngine format to dx12 format
DXGI_FORMAT actualFormat = convertToDXGIFormat(format);

uint32_t index;
TextureData &data = m_texturePool.getFreeMemoryData(index);
D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
if (allowWrite) {
  flags = flags | (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}
auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(actualFormat, width, height, 1, 1,
                                            1, 0, flags);

D3D12_CLEAR_VALUE clear;
clear.Color[0] = 0.0f;
clear.Color[1] = 0.0f;
clear.Color[2] = 0.0f;
clear.Color[3] = 1.0f;
clear.Format = actualFormat;
auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
HRESULT hr = dx12::DEVICE->CreateCommittedResource(
    &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc,
    D3D12_RESOURCE_STATE_RENDER_TARGET, &clear, IID_PPV_ARGS(&data.resource));
assert(SUCCEEDED(hr));

data.magicNumber = MAGIC_NUMBER_COUNTER;
data.format = data.resource->GetDesc().Format;
data.state = D3D12_RESOURCE_STATE_RENDER_TARGET;
data.flags = TextureFlags::RT;

TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

++MAGIC_NUMBER_COUNTER;

createRTVSRV(dx12::GLOBAL_RTV_HEAP, data.resource, data.rtsrv);
dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(data.srv, data.resource,
                                                  data.format);
if (allowWrite) {
  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DUAV(data.uav, data.resource,
                                                    data.format);
}

// convert to wstring
const std::string sname(name);
const std::wstring wname(sname.begin(), sname.end());
data.resource->SetName(wname.c_str());

m_nameToHandle[name] = handle;
return handle;
*/
  return {};
}

TextureHandle VkTextureManager::allocateTexture(const uint32_t width,
                                                const uint32_t height,
                                                const RenderTargetFormat format,
                                                const char *name,
                                                const bool mips,
                                                const bool allowWrite) {
  /*
// convert SirEngine format to dx12 format
const DXGI_FORMAT actualFormat = convertToDXGIFormat(format);

uint32_t index;
TextureData &data = m_texturePool.getFreeMemoryData(index);
D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
if (allowWrite) {
  flags = flags | (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

const uint16_t mipsLevel = mips ? static_cast<uint16_t>(std::log2(width)) : 1;
auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(actualFormat, width, height, 1,
                                            mipsLevel, 1, 0, flags);

const D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
HRESULT hr = dx12::DEVICE->CreateCommittedResource(
    &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, state, nullptr,
    IID_PPV_ARGS(&data.resource));
assert(SUCCEEDED(hr));

data.magicNumber = MAGIC_NUMBER_COUNTER;
data.format = data.resource->GetDesc().Format;
data.state = state;
data.flags = static_cast<TextureFlags>(0);

const TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

++MAGIC_NUMBER_COUNTER;

dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(data.srv, data.resource,
                                                  data.format);
if (allowWrite) {
  dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DUAV(data.uav, data.resource,
                                                    data.format);
}

// convert to wstring
const std::string sname(name);
const std::wstring wname(sname.begin(), sname.end());
data.resource->SetName(wname.c_str());

m_nameToHandle[name] = handle;
return handle;*/
  return {};
}

void VkTextureManager::bindRenderTarget(const TextureHandle handle,
                                        const TextureHandle depth) {
  /*
assertMagicNumber(handle);
const uint32_t index = getIndexFromHandle(handle);
const TextureData &data = m_texturePool.getConstRef(index);
assert((data.flags & TextureFlags::RT) > 0);

auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
auto commandList = currentFc->commandList;

D3D12_CPU_DESCRIPTOR_HANDLE handles[1] = {data.rtsrv.cpuHandle};
// TODO fix this, should not have a depth the swap chain??
// auto backDepth = dx12::SWAP_CHAIN->getDepthCPUDescriptor();
const D3D12_CPU_DESCRIPTOR_HANDLE *depthDesc = nullptr;
if (depth.isHandleValid()) {
  assertMagicNumber(depth);
  const uint32_t depthIndex = getIndexFromHandle(depth);
  const TextureData &depthData = m_texturePool.getConstRef(depthIndex);
  assert((depthData.flags & TextureFlags::DEPTH) > 0);
  depthDesc = &(depthData.rtsrv.cpuHandle);
}
commandList->OMSetRenderTargets(1, handles, true, depthDesc);
*/
}

void VkTextureManager::bindRenderTargetStencil(TextureHandle handle,
                                               TextureHandle depth) {
  /*
assertMagicNumber(handle);
const uint32_t index = getIndexFromHandle(handle);
const TextureData &data = m_texturePool.getConstRef(index);
assert((data.flags & TextureFlags::RT) > 0);

auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
auto commandList = currentFc->commandList;

D3D12_CPU_DESCRIPTOR_HANDLE handles[1] = {data.rtsrv.cpuHandle};
// TODO fix this, should not have a depth the swap chain??
// auto backDepth = dx12::SWAP_CHAIN->getDepthCPUDescriptor();
const D3D12_CPU_DESCRIPTOR_HANDLE *depthDesc = nullptr;
if (depth.isHandleValid()) {
  assertMagicNumber(depth);
  const uint32_t depthIndex = getIndexFromHandle(depth);
  const TextureData &depthData = m_texturePool.getConstRef(depthIndex);
  assert((depthData.flags & TextureFlags::DEPTH) > 0);
  depthDesc = &(depthData.dsvStencil.cpuHandle);
}
commandList->OMSetRenderTargets(1, handles, true, depthDesc);
*/
}

void VkTextureManager::copyTexture(const TextureHandle source,
                                   const TextureHandle destination) {
  /*
assertMagicNumber(source);
assertMagicNumber(destination);

const uint32_t sourceIdx = getIndexFromHandle(source);
const uint32_t destIdx = getIndexFromHandle(destination);

TextureData &sourceData = m_texturePool[sourceIdx];
TextureData &destData = m_texturePool[destIdx];

auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
auto commandList = currentFc->commandList;

D3D12_RESOURCE_BARRIER barriers[2];

int counter = 0;
auto state = sourceData.state;
if (sourceData.state != D3D12_RESOURCE_STATE_COPY_SOURCE) {
  barriers[counter] = CD3DX12_RESOURCE_BARRIER::Transition(
      sourceData.resource, state, D3D12_RESOURCE_STATE_COPY_SOURCE);
  sourceData.state = D3D12_RESOURCE_STATE_COPY_SOURCE;
  ++counter;
}
state = destData.state;
if (destData.state != D3D12_RESOURCE_STATE_COPY_DEST) {
  barriers[counter] = CD3DX12_RESOURCE_BARRIER::Transition(
      destData.resource, state, D3D12_RESOURCE_STATE_COPY_DEST);
  destData.state = D3D12_RESOURCE_STATE_COPY_DEST;
  ++counter;
}
if (counter > 0) {
  commandList->ResourceBarrier(counter, barriers);
}
commandList->CopyResource(destData.resource, sourceData.resource);
*/
}

void VkTextureManager::bindBackBuffer(bool bindBackBufferDepth) {
  /*
   *auto back = dx12::SWAP_CHAIN->currentBackBufferView();
  auto depth = dx12::SWAP_CHAIN->getDepthCPUDescriptor();
  auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
  auto commandList = currentFc->commandList;
  commandList->OMSetRenderTargets(1, &back, true,
                                  bindBackBufferDepth ? &depth : nullptr);
  ;
  */
}

void VkTextureManager::clearDepth(const TextureHandle depth,
                                  const float value) {
  /*
assertMagicNumber(depth);
const uint32_t index = getIndexFromHandle(depth);
const TextureData &data = m_texturePool.getConstRef(index);
assert((data.flags & TextureFlags::DEPTH) > 0);

CURRENT_FRAME_RESOURCE->fc.commandList->ClearDepthStencilView(
    data.rtsrv.cpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
    value, 0, 0, nullptr);
    */
}

void VkTextureManager::clearRT(const TextureHandle handle,
                               const float color[4]) {
  /*
assertMagicNumber(handle);
const uint32_t index = getIndexFromHandle(handle);
const TextureData &data = m_texturePool.getConstRef(index);
assert((data.flags & TextureFlags::RT) > 0);
// Clear the back buffer and depth buffer.
CURRENT_FRAME_RESOURCE->fc.commandList->ClearRenderTargetView(
    data.rtsrv.cpuHandle, color, 0, nullptr);
    */
}

void VkTextureManager::initialize() {
  m_whiteTexture = loadTexture(WHITE_TEXTURE_PATH, false);
}

void VkTextureManager::cleanup() { free(m_whiteTexture); }
} // namespace SirEngine::vk