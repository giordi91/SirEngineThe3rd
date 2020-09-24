#include "platform/windows/graphics/vk/vkSwapChain.h"
#include "SirEngine/globals.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkLoad.h"
#include <algorithm>
#include <cassert>
#include <iostream>

#include "vkBindingTableManager.h"

namespace SirEngine::vk {

bool selectDesiredPresentationMode(const VkPhysicalDevice physicalDevice,
                                   const VkSurfaceKHR presentationSurface,
                                   const VkPresentModeKHR desiredPresentMode,
                                   VkPresentModeKHR &presentMode) {
  // Enumerate supported present modes
  uint32_t presentModesCount = 0;
  VkResult result = VK_SUCCESS;

  result = vkGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice, presentationSurface, &presentModesCount, nullptr);
  if ((VK_SUCCESS != result) || (0 == presentModesCount)) {
    std::cout << "Could not get the number of supported present modes."
              << std::endl;
    return false;
  }

  std::vector<VkPresentModeKHR> presentModes(presentModesCount);
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice, presentationSurface, &presentModesCount,
      presentModes.data());
  if ((VK_SUCCESS != result) || (0 == presentModesCount)) {
    std::cout << "Could not enumerate present modes." << std::endl;
    return false;
  }

  // Select present mode
  for (auto &currentPresentMode : presentModes) {
    if (currentPresentMode == desiredPresentMode) {
      presentMode = desiredPresentMode;
      return true;
    }
  }

  std::cout
      << "Desired present mode is not supported. Selecting default FIFO mode."
      << std::endl;
  for (auto &currentPresentMode : presentModes) {
    if (currentPresentMode == VK_PRESENT_MODE_FIFO_KHR) {
      presentMode = VK_PRESENT_MODE_FIFO_KHR;
      return true;
    }
  }

  std::cout << "VK_PRESENT_MODE_FIFO_KHR is not supported though it's "
               "mandatory for all drivers!"
            << std::endl;
  return false;
}
bool getCapabilitiesOfPresentationSurface(
    const VkPhysicalDevice physicalDevice,
    const VkSurfaceKHR presentationSurface,
    VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  const VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physicalDevice, presentationSurface, &surfaceCapabilities);

  if (VK_SUCCESS != result) {
    std::cout << "Could not get the capabilities of a presentation surface."
              << std::endl;
    return false;
  }
  return true;
}

bool selectNumberOfSwapchainImages(
    VkSurfaceCapabilitiesKHR const &surfaceCapabilities,
    uint32_t &numberOfImages) {

  // here we clamp our value of images wanted expressed in the config, such that
  // must be least/equal than max and min, but if in range keeping our wanted
  // value
  uint32_t finalImageCount =
      std::max(std::min(surfaceCapabilities.maxImageCount,
                        globals::ENGINE_CONFIG->m_frameBufferingCount),
               surfaceCapabilities.minImageCount);
  if (finalImageCount != globals::ENGINE_CONFIG->m_frameBufferingCount) {
    SE_CORE_WARN("Swap chain: could not create requested number of images:{0}, "
                 "created {1} instead",
                 globals::ENGINE_CONFIG->m_frameBufferingCount,
                 finalImageCount);
  }
  numberOfImages = finalImageCount;

  return true;
}

bool chooseSizeOfSwapchainImages(
    VkSurfaceCapabilitiesKHR const &surfaceCapabilities,
    VkExtent2D &sizeOfImages) {
  if (0xFFFFFFFF == surfaceCapabilities.currentExtent.width) {
    sizeOfImages = {640, 480};

    if (sizeOfImages.width < surfaceCapabilities.minImageExtent.width) {
      sizeOfImages.width = surfaceCapabilities.minImageExtent.width;
    } else if (sizeOfImages.width > surfaceCapabilities.maxImageExtent.width) {
      sizeOfImages.width = surfaceCapabilities.maxImageExtent.width;
    }

    if (sizeOfImages.height < surfaceCapabilities.minImageExtent.height) {
      sizeOfImages.height = surfaceCapabilities.minImageExtent.height;
    } else if (sizeOfImages.height >
               surfaceCapabilities.maxImageExtent.height) {
      sizeOfImages.height = surfaceCapabilities.maxImageExtent.height;
    }
  } else {
    sizeOfImages = surfaceCapabilities.currentExtent;
  }
  return true;
}

bool selectDesiredUsageScenariosOfSwapchainImages(
    VkSurfaceCapabilitiesKHR const &surfaceCapabilities,
    const VkImageUsageFlags desiredUsages, VkImageUsageFlags &imageUsage) {
  imageUsage = desiredUsages & surfaceCapabilities.supportedUsageFlags;

  return desiredUsages == imageUsage;
}

auto SelectTransformationOfSwapchainImages(
    VkSurfaceCapabilitiesKHR const &surfaceCapabilities,
    const VkSurfaceTransformFlagBitsKHR desiredTransform,
    VkSurfaceTransformFlagBitsKHR &surfaceTransform) -> bool {
  if (surfaceCapabilities.supportedTransforms & desiredTransform) {
    surfaceTransform = desiredTransform;
  } else {
    surfaceTransform = surfaceCapabilities.currentTransform;
  }
  return true;
}

bool selectFormatOfSwapchainImages(
    const VkPhysicalDevice physicalDevice,
    const VkSurfaceKHR presentationSurface,
    const VkSurfaceFormatKHR desiredSurfaceFormat, VkFormat &imageFormat,
    VkColorSpaceKHR &imageColorSpace) {
  // Enumerate supported formats
  uint32_t formatsCount = 0;
  VkResult result = VK_SUCCESS;

  result = vkGetPhysicalDeviceSurfaceFormatsKHR(
      physicalDevice, presentationSurface, &formatsCount, nullptr);
  if ((VK_SUCCESS != result) || (0 == formatsCount)) {
    std::cout << "Could not get the number of supported surface formats."
              << std::endl;
    return false;
  }

  std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(
      physicalDevice, presentationSurface, &formatsCount,
      surfaceFormats.data());
  if ((VK_SUCCESS != result) || (0 == formatsCount)) {
    std::cout << "Could not enumerate supported surface formats." << std::endl;
    return false;
  }

  // weird corner case of the specs, if one format is return and is undefined,
  // it means we can use any format we like, no question asked
  if ((1 == surfaceFormats.size()) &&
      (VK_FORMAT_UNDEFINED == surfaceFormats[0].format)) {
    imageFormat = desiredSurfaceFormat.format;
    imageColorSpace = desiredSurfaceFormat.colorSpace;
    return true;
  }

  // Select surface format
  for (auto &surfaceFormat : surfaceFormats) {
    if ((desiredSurfaceFormat.format == surfaceFormat.format) &&
        (desiredSurfaceFormat.colorSpace == surfaceFormat.colorSpace)) {
      imageFormat = desiredSurfaceFormat.format;
      imageColorSpace = desiredSurfaceFormat.colorSpace;
      return true;
    }
  }

  for (auto &surfaceFormat : surfaceFormats) {
    if ((desiredSurfaceFormat.format == surfaceFormat.format)) {
      imageFormat = desiredSurfaceFormat.format;
      imageColorSpace = surfaceFormat.colorSpace;
      std::cout << "Desired combination of format and colorspace is not "
                   "supported. Selecting other colorspace."
                << std::endl;
      return true;
    }
  }

  imageFormat = surfaceFormats[0].format;
  imageColorSpace = surfaceFormats[0].colorSpace;
  std::cout << "Desired format is not supported. Selecting available format - "
               "colorspace combination."
            << std::endl;
  return true;
}

VkCompositeAlphaFlagBitsKHR getAlphaComposite(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface) {

  // we want to check what alpha transform/composition is supported
  VkSurfaceCapabilitiesKHR surfCaps;
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                                     &surfCaps));
  VkCompositeAlphaFlagBitsKHR surfaceComposites =
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  if (surfCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
    surfaceComposites = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  } else if (surfCaps.supportedCompositeAlpha &
             VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
    surfaceComposites = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
  } else if (surfCaps.supportedCompositeAlpha &
             VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
    surfaceComposites = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
  } else if (surfCaps.supportedCompositeAlpha &
             VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
    surfaceComposites = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  }
  return surfaceComposites;
}

bool getHandlesOfSwapchainImages(VkDevice logical_device,
                                 VkSwapchainKHR swapchain,
                                 std::vector<VkImage> &swapchainImages) {
  uint32_t imagesCount = 0;
  VkResult result = VK_SUCCESS;

  result =
      vkGetSwapchainImagesKHR(logical_device, swapchain, &imagesCount, nullptr);
  if ((VK_SUCCESS != result) || (0 == imagesCount)) {
    std::cout << "Could not get the number of swapchain images." << std::endl;
    return false;
  }

  swapchainImages.resize(imagesCount);
  result = vkGetSwapchainImagesKHR(logical_device, swapchain, &imagesCount,
                                   swapchainImages.data());
  if ((VK_SUCCESS != result) || (0 == imagesCount)) {
    std::cout << "Could not enumerate swapchain images." << std::endl;
    return false;
  }

  return true;
}

VkImageView createSwapchainImageView(const VkDevice logicalDevice,
                                     const VkImage swapchainImage) {
  VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  createInfo.image = swapchainImage;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = vk::IMAGE_FORMAT;
  createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  createInfo.subresourceRange.layerCount = 1;
  createInfo.subresourceRange.levelCount = 1;
  VkImageView view;
  vkCreateImageView(logicalDevice, &createInfo, nullptr, &view);
  return view;
}

bool createSwapchainWithR8G8B8A8FormatAndMailboxPresentMode(
    const VkPhysicalDevice physicalDevice,
    const VkSurfaceKHR presentationSurface, const VkDevice logicalDevice,
    const VkImageUsageFlags swapchainImageUsage, VkExtent2D &imageSize,
    VkFormat &imageFormat, VkSwapchainKHR &oldSwapchain,
    VkSwapchainKHR &swapchain, std::vector<VkImage> &swapchainImages,
    std::vector<VkImageView> &swapchainImageViews) {

  VkPresentModeKHR desiredPresentMode;
  // TODO should support v-sync
  if (!selectDesiredPresentationMode(physicalDevice, presentationSurface,
                                     VK_PRESENT_MODE_IMMEDIATE_KHR,
                                     // VK_PRESENT_MODE_FIFO_KHR,
                                     desiredPresentMode)) {
    return false;
  }

  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  if (!getCapabilitiesOfPresentationSurface(physicalDevice, presentationSurface,
                                            surfaceCapabilities)) {
    return false;
  }

  if (!selectNumberOfSwapchainImages(surfaceCapabilities,
                                     SWAP_CHAIN_IMAGE_COUNT)) {
    return false;
  }

  if (!chooseSizeOfSwapchainImages(surfaceCapabilities, imageSize)) {
    return false;
  }
  if ((0 == imageSize.width) || (0 == imageSize.height)) {
    return true;
  }

  VkImageUsageFlags imageUsage;
  if (!selectDesiredUsageScenariosOfSwapchainImages(
          surfaceCapabilities, swapchainImageUsage, imageUsage)) {
    return false;
  }

  VkSurfaceTransformFlagBitsKHR surfaceTransform;
  SelectTransformationOfSwapchainImages(surfaceCapabilities,
                                        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                                        surfaceTransform);

  VkColorSpaceKHR imageColorSpace;
  if (!selectFormatOfSwapchainImages(
          physicalDevice, presentationSurface,
          {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
          imageFormat, imageColorSpace)) {
    return false;
  }
  const VkCompositeAlphaFlagBitsKHR surfaceComposites =
      getAlphaComposite(physicalDevice, presentationSurface);

  // creating the actual swap chain:
  VkSwapchainCreateInfoKHR swapchainCreateInfo = {
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      nullptr,
      0,
      presentationSurface,
      SWAP_CHAIN_IMAGE_COUNT,
      imageFormat,
      imageColorSpace,
      imageSize,
      1,
      imageUsage,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr,
      surfaceTransform,
      surfaceComposites,
      desiredPresentMode,
      VK_TRUE,
      oldSwapchain};

  const VkResult result = vkCreateSwapchainKHR(
      logicalDevice, &swapchainCreateInfo, nullptr, &swapchain);
  if ((VK_SUCCESS != result) || (VK_NULL_HANDLE == swapchain)) {
    std::cout << "Could not create a swapchain." << std::endl;
    return false;
  }

  // if (VK_NULL_HANDLE != oldSwapchain) {
  //  //TODO revisit
  //  vkDestroySwapchainKHR(logicalDevice, oldSwapchain, nullptr);
  //  oldSwapchain = VK_NULL_HANDLE;
  //}

  if (!getHandlesOfSwapchainImages(logicalDevice, swapchain, swapchainImages)) {
    return false;
  }
  // now that we have the swap chain images we need to create the view to them
  swapchainImageViews.resize(SWAP_CHAIN_IMAGE_COUNT);
  assert(SWAP_CHAIN_IMAGE_COUNT < 9);
  char charNumber[2];
  charNumber[1] = '\0';
  for (uint32_t i = 0; i < SWAP_CHAIN_IMAGE_COUNT; ++i) {
    charNumber[0] = static_cast<char>(48 + i);
    swapchainImageViews[i] =
        createSwapchainImageView(logicalDevice, swapchainImages[i]);
    SET_DEBUG_NAME(swapchainImages[i], VK_OBJECT_TYPE_IMAGE,
                   frameConcatenation("swapChainImage", charNumber));
  }

  return true;
}

bool createSwapchain(const VkDevice logicalDevice,
                     const VkPhysicalDevice physicalDevice,
                     const VkSurfaceKHR surface, const uint32_t width,
                     const uint32_t height, VkSwapchain *oldSwapchain,
                     VkSwapchain &outSwapchain) {

  VK_CHECK(vkDeviceWaitIdle(logicalDevice));

  // Ready = false;

  VkSwapchainKHR old =
      oldSwapchain == nullptr ? VK_NULL_HANDLE : oldSwapchain->swapchain;
  VkExtent2D swapchainImageSize;
  if (!createSwapchainWithR8G8B8A8FormatAndMailboxPresentMode(
          physicalDevice, surface, logicalDevice,
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
          swapchainImageSize, vk::IMAGE_FORMAT, old, outSwapchain.swapchain,
          outSwapchain.images, outSwapchain.imagesView)) {
    return false;
  }
  outSwapchain.width = width;
  outSwapchain.height = height;

  // Swap chain render pass
  VkRenderPass renderPass{};

  VkAttachmentDescription attachments[1] = {};
  attachments[0].format = vk::IMAGE_FORMAT;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // the index here, 0, refers to the index in the attachment array;
  VkAttachmentReference attachReference{
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subPass{};
  subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subPass.colorAttachmentCount = 1;
  subPass.pColorAttachments = &attachReference;

  VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  createInfo.attachmentCount = 1;
  createInfo.pAttachments = attachments;
  createInfo.subpassCount = 1;
  createInfo.pSubpasses = &subPass;

  VK_CHECK(
      vkCreateRenderPass(logicalDevice, &createInfo, nullptr, &renderPass));
  SET_DEBUG_NAME(renderPass, VK_OBJECT_TYPE_RENDER_PASS, "swapChainRenderPass");

  outSwapchain.renderPass = renderPass;

  // create the frame buffer
  const size_t count = outSwapchain.images.size();
  outSwapchain.frameBuffers.resize(count);
  for (size_t i = 0; i < count; ++i) {
    outSwapchain.frameBuffers[i] =
        createFrameBuffer(logicalDevice, outSwapchain.renderPass,
                          outSwapchain.imagesView[i], width, height);
  }
  // createFrameBuffer(LOGICAL_DEVICE, FRAME_BUFFERS);

  if (oldSwapchain != nullptr) {
    destroySwapchain(logicalDevice, oldSwapchain);
  }

  assert(outSwapchain.swapchain != nullptr);
  return true;
}

bool destroySwapchain(const VkDevice logicalDevice, VkSwapchain *swapchain) {
  if (swapchain->swapchain == nullptr) {
    return true;
  }
  for (auto &img : swapchain->imagesView) {
    vkDestroyImageView(logicalDevice, img, nullptr);
  }
  for (auto &fb : swapchain->frameBuffers) {
    vkDestroyFramebuffer(logicalDevice, fb, nullptr);
  }

  vkDestroyRenderPass(logicalDevice, swapchain->renderPass, nullptr);

  vkDestroySwapchainKHR(logicalDevice, swapchain->swapchain, nullptr);
  delete swapchain;
  return true;
}

void resizeSwapchain(const VkDevice logicalDevice,
                     const VkPhysicalDevice physicalDevice,
                     VkSurfaceKHR surface, uint32_t width, uint32_t height,
                     VkSwapchain &outSwapchain) {
  VkSwapchain old = outSwapchain;
  createSwapchain(logicalDevice, physicalDevice, surface, width, height, &old,
                  outSwapchain);
}
} // namespace SirEngine::vk
