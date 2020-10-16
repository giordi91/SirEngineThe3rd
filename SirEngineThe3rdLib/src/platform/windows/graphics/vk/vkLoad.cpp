
#include "platform/windows/graphics/vk/vkLoad.h"

#include <cassert>

#include "SirEngine/log.h"
#include "platform/windows/graphics/vk/volk.h"
#include "vk.h"

namespace SirEngine::vk {

VkBool32 VKAPI_PTR debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
  const char *type =
      messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
          ? "ERROR"
          : (messageSeverity &
             (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT))
                ? "WARNING"
                : "INFO";

  char message[4096];
  snprintf(message, ARRAYSIZE(message), "%s: %s\n", type,
           pCallbackData->pMessage);
#if _WIN32
  OutputDebugStringA(message);
#endif

  printf("%s", message);
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
     assert(0 && "validation layer assertion");
  }

  // always need to return false, true is reserved for layer development
  return VK_FALSE;
}

bool isExtensionSupported(
    std::vector<VkExtensionProperties> const &availableExtensions,
    char const *const extension) {
  for (const auto &availableExtension : availableExtensions) {
    if (strstr(availableExtension.extensionName, extension)) {
      return true;
    }
  }
  return false;
}

bool checkAvailableInstanceExtensions(
    std::vector<VkExtensionProperties> &availableExtensions) {
  uint32_t extensionsCount = 0;

  VkResult result = vkEnumerateInstanceExtensionProperties(
      nullptr, &extensionsCount, nullptr);
  if ((result != VK_SUCCESS) || (extensionsCount == 0)) {
    SE_CORE_ERROR("Could not get the number of instance extensions.");
    return false;
  }

  availableExtensions.resize(extensionsCount);
  result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount,
                                                  availableExtensions.data());
  if ((result != VK_SUCCESS) || (extensionsCount == 0)) {
    SE_CORE_ERROR("Could not enumerate instance extensions.");
    return false;
  }

  return true;
}

bool createVulkanInstance(std::vector<char const *> const &desiredExtensions,
                          char const *const applicationName,
                          VkInstance &instance) {
  std::vector<VkExtensionProperties> availableExtensions;
  if (!checkAvailableInstanceExtensions(availableExtensions)) {
    return false;
  }

  for (auto &extension : desiredExtensions) {
    if (!isExtensionSupported(availableExtensions, extension)) {
      SE_CORE_ERROR(
          "Extension named '{0}'is not supported by an Instance object.",
          extension);
      return false;
    }
  }

  VkApplicationInfo applicationInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                       nullptr,
                                       applicationName,
                                       VK_MAKE_VERSION(1, 0, 0),
                                       "Sir Engine",
                                       VK_MAKE_VERSION(1, 0, 0),
                                       VK_API_VERSION_1_1};

  VkInstanceCreateInfo instanceCreateInfo = {
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      nullptr,
      0,
      &applicationInfo,
      0,
      nullptr,
      static_cast<uint32_t>(desiredExtensions.size()),
      desiredExtensions.data()};

#if _DEBUG
  const char *layers[] = {
      // NOTE Khronos layer name changed with 1.135 Vulkan SDK
      // VK_LAYER_LUNARG_standard_validation"
      "VK_LAYER_KHRONOS_validation",
  };
  instanceCreateInfo.ppEnabledLayerNames = layers;
  instanceCreateInfo.enabledLayerCount = ARRAYSIZE(layers);
#endif

  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
  SE_CORE_INFO("Available layers: ");
  for (auto l : availableLayers) {
    SE_CORE_INFO(l.layerName);
  }

  /*
  VkValidationFeatureEnableEXT enables[] = {
      VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
      VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
  };
  VkValidationFeaturesEXT features = {};
  features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
  features.enabledValidationFeatureCount = ARRAYSIZE(enables);
  features.pEnabledValidationFeatures = enables;
  instanceCreateInfo.pNext = &features;
  */

  instanceCreateInfo.pNext = nullptr;
  const VkResult result =
      vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
  if ((result != VK_SUCCESS) || (instance == VK_NULL_HANDLE)) {
    SE_CORE_ERROR("Could not create Vulkan instance.");
    return false;
  }

  return true;
}

bool registerDebugCallback(VkInstance instance) {
  // create debug utils messenger
  VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = {};
  debug_utils_messenger_create_info.sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debug_utils_messenger_create_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
  debug_utils_messenger_create_info.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  debug_utils_messenger_create_info.pfnUserCallback = debugCallback;
  if (VK_SUCCESS !=
      vkCreateDebugUtilsMessengerEXT(
          instance, &debug_utils_messenger_create_info, NULL, &DEBUG_CALLBACK2))
    exit(EXIT_FAILURE);
  return true;
}

bool createVulkanInstanceWithWsiExtensionsEnabled(
    std::vector<char const *> &desiredExtensions,
    char const *const applicationName, VkInstance &instance) {
  desiredExtensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
  desiredExtensions.emplace_back(
#ifdef VK_USE_PLATFORM_WIN32_KHR
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME

#elif defined VK_USE_PLATFORM_XCB_KHR
      VK_KHR_XCB_SURFACE_EXTENSION_NAME

#elif defined VK_USE_PLATFORM_XLIB_KHR
      VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#endif
  );
  // used for validation layer error DEBUG_CALLBACK
  desiredExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  desiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return createVulkanInstance(desiredExtensions, applicationName, instance);
}

bool enumerateAvailablePhysicalDevices(
    const VkInstance instance,
    std::vector<VkPhysicalDevice> &availableDevices) {
  uint32_t devicesCount = 0;
  VkResult result = VK_SUCCESS;

  // first time you pass a null pointer so you can ask the size, then resize the
  // memory and finally you can query
  result = vkEnumeratePhysicalDevices(instance, &devicesCount, nullptr);
  if ((result != VK_SUCCESS) || (devicesCount == 0)) {
    SE_CORE_ERROR("Could not get the number of available physical devices.");
    return false;
  }

  availableDevices.resize(devicesCount);
  result = vkEnumeratePhysicalDevices(instance, &devicesCount,
                                      availableDevices.data());
  if ((result != VK_SUCCESS) || (devicesCount == 0)) {
    SE_CORE_ERROR("Could not enumerate physical devices.");
    return false;
  }

  VkPhysicalDeviceProperties properties;
  SE_CORE_INFO("Available devices: \n");
  for (int i = 0; i < availableDevices.size(); ++i) {
    vkGetPhysicalDeviceProperties(availableDevices[i], &properties);
    SE_CORE_INFO("\n {}", properties.deviceName);
  }

  return true;
}
void getFeaturesAndPropertiesOfPhysicalDevice(
    const VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures2 &deviceFeatures,
    VkPhysicalDeviceProperties &deviceProperties) {
  vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);

  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
}

bool checkAvailableQueueFamiliesAndTheirProperties(
    const VkPhysicalDevice physicalDevice,
    std::vector<VkQueueFamilyProperties> &queueFamilies) {
  uint32_t queueFamiliesCount = 0;

  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount,
                                           nullptr);
  if (queueFamiliesCount == 0) {
    SE_CORE_ERROR("Could not get the number of queue families.");
    return false;
  }

  queueFamilies.resize(queueFamiliesCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount,
                                           queueFamilies.data());
  if (queueFamiliesCount == 0) {
    SE_CORE_ERROR("Could not acquire properties of queue families.");
    return false;
  }

  return true;
}

bool selectIndexOfQueueFamilyWithDesiredCapabilities(
    const VkPhysicalDevice physicalDevice,
    const VkQueueFlags desiredCapabilities, uint32_t &queueFamilyIndex) {
  std::vector<VkQueueFamilyProperties> queueFamilies;
  if (!checkAvailableQueueFamiliesAndTheirProperties(physicalDevice,
                                                     queueFamilies)) {
    return false;
  }

  for (uint32_t index = 0; index < static_cast<uint32_t>(queueFamilies.size());
       ++index) {
    if ((queueFamilies[index].queueCount > 0) &&
        ((queueFamilies[index].queueFlags & desiredCapabilities) ==
         desiredCapabilities)) {
      queueFamilyIndex = index;
      return true;
    }
  }
  return false;
}

bool checkAvailableDeviceExtensions(
    const VkPhysicalDevice physicalDevice,
    std::vector<VkExtensionProperties> &availableExtensions) {
  uint32_t extensionsCount = 0;
  VkResult result = VK_SUCCESS;

  result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                                &extensionsCount, nullptr);
  if ((result != VK_SUCCESS) || (extensionsCount == 0)) {
    SE_CORE_ERROR("Could not get the number of device extensions.");
    return false;
  }

  availableExtensions.resize(extensionsCount);
  result = vkEnumerateDeviceExtensionProperties(
      physicalDevice, nullptr, &extensionsCount, availableExtensions.data());
  if ((result != VK_SUCCESS) || (extensionsCount == 0)) {
    SE_CORE_ERROR("Could not enumerate device extensions.");
    return false;
  }

  return true;
}

bool createLogicalDevice(const VkPhysicalDevice physicalDevice,
                         std::vector<QueueInfo> queueInfos,
                         std::vector<char const *> const &desiredExtensions,
                         VkPhysicalDeviceFeatures2 *desiredFeatures,
                         VkDevice &logicalDevice) {
  std::vector<VkExtensionProperties> availableExtensions;
  if (!checkAvailableDeviceExtensions(physicalDevice, availableExtensions)) {
    return false;
  }

  for (auto &extension : desiredExtensions) {
    if (!isExtensionSupported(availableExtensions, extension)) {
      SE_CORE_ERROR(
          "Extension named '{}' is not supported by a physical device.",
          extension);
      return false;
    }
  }

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

  queueCreateInfos.reserve(queueInfos.size());
  for (auto &info : queueInfos) {
    queueCreateInfos.push_back({VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                nullptr, 0, info.familyIndex,
                                static_cast<uint32_t>(info.priorities.size()),
                                info.priorities.data()});
  };

  VkDeviceCreateInfo deviceCreateInfo = {
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      desiredFeatures,
      0,
      static_cast<uint32_t>(queueCreateInfos.size()),
      queueCreateInfos.data(),
      0,
      nullptr,
      static_cast<uint32_t>(desiredExtensions.size()),
      desiredExtensions.data()};

  const VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo,
                                         nullptr, &logicalDevice);
  if ((result != VK_SUCCESS) || (logicalDevice == VK_NULL_HANDLE)) {
    SE_CORE_ERROR("Could not create logical device.");
    return false;
  }

  return true;
}

void getDeviceQueue(const VkDevice logicalDevice,
                    const uint32_t queueFamilyIndex, const uint32_t queueIndex,
                    VkQueue &queue) {
  vkGetDeviceQueue(logicalDevice, queueFamilyIndex, queueIndex, &queue);
}

bool selectQueueFamilyThatSupportsPresentationToGivenSurface(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR presentationSurface,
    uint32_t &queueFamilyIndex) {
  std::vector<VkQueueFamilyProperties> queue_families;
  if (!checkAvailableQueueFamiliesAndTheirProperties(physicalDevice,
                                                     queue_families)) {
    return false;
  }

  for (uint32_t index = 0; index < static_cast<uint32_t>(queue_families.size());
       ++index) {
    VkBool32 presentation_supported = VK_FALSE;
    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(
        physicalDevice, index, presentationSurface, &presentation_supported);
    if ((VK_SUCCESS == result) && (VK_TRUE == presentation_supported)) {
      queueFamilyIndex = index;
      return true;
    }
  }
  return false;
}

bool newSemaphore(const VkDevice logicalDevice, VkSemaphore &semaphore) {
  VkSemaphoreCreateInfo semaphoreCreateInfo = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};

  const VkResult result = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo,
                                            nullptr, &semaphore);
  if (VK_SUCCESS != result) {
    SE_CORE_ERROR("Could not create a semaphore.");
    return false;
  }
  return true;
}

bool presentImage(VkQueue queue, std::vector<VkSemaphore> renderingSemaphores,
                  std::vector<PresentInfo> imagesToPresent) {
  std::vector<VkSwapchainKHR> swapchains;
  std::vector<uint32_t> imageIndices;

  for (auto &imageToPresent : imagesToPresent) {
    swapchains.emplace_back(imageToPresent.swapchain);
    imageIndices.emplace_back(imageToPresent.imageIndex);
  }

  VkPresentInfoKHR presentInfo = {
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      nullptr,
      static_cast<uint32_t>(renderingSemaphores.size()),
      renderingSemaphores.data(),
      static_cast<uint32_t>(swapchains.size()),
      swapchains.data(),
      imageIndices.data(),
      nullptr};

  const VkResult result = vkQueuePresentKHR(queue, &presentInfo);
  switch (result) {
    case VK_SUCCESS:
      return true;
    default:
      return false;
  }
}

void setImageMemoryBarrier(
    const VkCommandBuffer commandBuffer,
    const VkPipelineStageFlags generatingStages,
    const VkPipelineStageFlags consumingStages,
    const std::vector<ImageTransition> imageTransitions) {
  std::vector<VkImageMemoryBarrier> imageMemoryBarriers;

  for (auto &imageTransition : imageTransitions) {
    imageMemoryBarriers.push_back(
        {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
         nullptr,
         imageTransition.currentAccess,
         imageTransition.newAccess,
         imageTransition.currentLayout,
         imageTransition.newLayout,
         imageTransition.currentQueueFamily,
         imageTransition.newQueueFamily,
         imageTransition.image,
         {

             imageTransition.aspect, 0, VK_REMAINING_MIP_LEVELS, 0,
             VK_REMAINING_ARRAY_LAYERS}});
  }

  if (!imageMemoryBarriers.empty()) {
    vkCmdPipelineBarrier(commandBuffer, generatingStages, consumingStages, 0, 0,
                         nullptr, 0, nullptr,
                         static_cast<uint32_t>(imageMemoryBarriers.size()),
                         imageMemoryBarriers.data());
  }
}

// framebuffer is the collection of images you are rendering to plus
// a couple of extra attributes
VkFramebuffer createFrameBuffer(const VkDevice logicalDevice,
                                const VkRenderPass renderPass,
                                VkImageView imageView, const uint32_t width,
                                const uint32_t height) {
  VkFramebufferCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  createInfo.renderPass = renderPass;
  createInfo.pAttachments = &imageView;
  createInfo.attachmentCount = 1;
  createInfo.width = width;
  createInfo.height = height;
  createInfo.layers = 1;

  VkFramebuffer frameBuffer = nullptr;
  vkCreateFramebuffer(logicalDevice, &createInfo, nullptr, &frameBuffer);
  return frameBuffer;
}

}  // namespace SirEngine::vk