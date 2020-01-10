
#include "platform/windows/graphics/vk/vkLoad.h"
#include "platform/windows/graphics/vk/volk.h"
//#include "platform/windows/graphics/vk/VulkanFunctions.h"
#include "SirEngine/log.h"
#include "vk.h"
#include <cassert>
#include <iostream>

namespace SirEngine {
namespace vk {

VkBool32 debugCallback(VkDebugReportFlagsEXT flags,
                       VkDebugReportObjectTypeEXT objectType, uint64_t object,
                       size_t location, int32_t messageCode,
                       const char *pLayerPrefix, const char *pMessage,
                       void *pUserData) {

  const char *type =
      flags & VK_DEBUG_REPORT_ERROR_BIT_EXT
          ? "ERROR"
          : (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
                ? "WARNING"
                : "INFO";

  char message[4096];
  snprintf(message, ARRAYSIZE(message), "%s: %s\n", type, pMessage);
#if _WIN32
  OutputDebugStringA(message);
#endif

  printf("%s", message);
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    assert(0 && "validation layer assertion");
  }

  // always need to return false, true is reserved for layer development
  return VK_FALSE;
}
VkBool32 VKAPI_PTR
debugCallback2(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
               void *pUserData) {

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
  for (auto &availableExtension : availableExtensions) {
    if (strstr(availableExtension.extensionName, extension)) {
      return true;
    }
  }
  return false;
}

bool checkAvailableInstanceExtensions(
    std::vector<VkExtensionProperties> &availableExtensions) {
  uint32_t extensionsCount = 0;

  VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount,
                                                           nullptr);
  if ((result != VK_SUCCESS) || (extensionsCount == 0)) {
    std::cout << "Could not get the number of instance extensions."
              << std::endl;
    return false;
  }

  availableExtensions.resize(extensionsCount);
  result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount,
                                                  availableExtensions.data());
  if ((result != VK_SUCCESS) || (extensionsCount == 0)) {
    std::cout << "Could not enumerate instance extensions." << std::endl;
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
      std::cout << "Extension named '" << extension
                << "' is not supported by an Instance object." << std::endl;
      return false;
    }
  }

  VkApplicationInfo applicationInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                       nullptr,
                                       applicationName,
                                       VK_MAKE_VERSION(1, 0, 0),
                                       "Vulkan Cookbook",
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
    "VK_LAYER_LUNARG_standard_validation",

#if VULKAN_OBJ_TRACKER
    "VK_LAYER_LUNARG_object_tracker",
#endif
#if VULKAN_PARAM_VALIDATION
    "VK_LAYER_LUNARG_parameter_validation",
#endif
  };
  instanceCreateInfo.ppEnabledLayerNames = layers;
  instanceCreateInfo.enabledLayerCount = ARRAYSIZE(layers);
#endif

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
    std::cout << "Could not create Vulkan instance." << std::endl;
    return false;
  }

  return true;
}

bool registerDebugCallback(VkInstance instance) {
  /*
VkDebugReportCallbackCreateInfoEXT createInfo{
VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT};
createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
               VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
               VK_DEBUG_REPORT_ERROR_BIT_EXT |
               VK_DEBUG_REPORT_DEBUG_BIT_EXT;
createInfo.pfnCallback = debugCallback;

const VkResult callbackResult = vkCreateDebugReportCallbackEXT(
INSTANCE, &createInfo, nullptr, &DEBUG_CALLBACK);
assert(callbackResult == VK_SUCCESS);
*/

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
  debug_utils_messenger_create_info.pfnUserCallback = debugCallback2;
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
    std::cout << "Could not get the number of available physical devices."
              << std::endl;
    return false;
  }

  availableDevices.resize(devicesCount);
  result = vkEnumeratePhysicalDevices(instance, &devicesCount,
                                      availableDevices.data());
  if ((result != VK_SUCCESS) || (devicesCount == 0)) {
    std::cout << "Could not enumerate physical devices." << std::endl;
    return false;
  }

  VkPhysicalDeviceProperties properties;
  std::cout << "Available devices: \n";
  for (int i = 0; i < availableDevices.size(); ++i) {
    vkGetPhysicalDeviceProperties(availableDevices[i], &properties);
    std::cout << properties.deviceName << "\n";
  }
  std::cout << std::endl;

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
    std::cout << "Could not get the number of queue families." << std::endl;
    return false;
  }

  queueFamilies.resize(queueFamiliesCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount,
                                           queueFamilies.data());
  if (queueFamiliesCount == 0) {
    std::cout << "Could not acquire properties of queue families." << std::endl;
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
    std::cout << "Could not get the number of device extensions." << std::endl;
    return false;
  }

  availableExtensions.resize(extensionsCount);
  result = vkEnumerateDeviceExtensionProperties(
      physicalDevice, nullptr, &extensionsCount, availableExtensions.data());
  if ((result != VK_SUCCESS) || (extensionsCount == 0)) {
    std::cout << "Could not enumerate device extensions." << std::endl;
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
      std::cout << "Extension named '" << extension
                << "' is not supported by a physical device." << std::endl;
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
    std::cout << "Could not create logical device." << std::endl;
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

bool createLogicalDeviceWithWsiExtensionsEnabled(
    const VkPhysicalDevice physicalDevice,
    const std::vector<QueueInfo> queueInfos,
    std::vector<char const *> &desiredExtensions,
    VkPhysicalDeviceFeatures2 *desiredFeatures, VkDevice &logicalDevice) {
  desiredExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  // desiredExtensions.emplace_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
  desiredExtensions.emplace_back(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
  desiredExtensions.emplace_back(VK_KHR_8BIT_STORAGE_EXTENSION_NAME);

  return createLogicalDevice(physicalDevice, queueInfos, desiredExtensions,
                             desiredFeatures, logicalDevice);
}
bool newSemaphore(const VkDevice logicalDevice, VkSemaphore &semaphore) {
  VkSemaphoreCreateInfo semaphoreCreateInfo = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};

  const VkResult result = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo,
                                            nullptr, &semaphore);
  if (VK_SUCCESS != result) {
    std::cout << "Could not create a semaphore." << std::endl;
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

bool beginCommandBufferRecordingOperation(
    const VkCommandBuffer commandBuffer, const VkCommandBufferUsageFlags usage,
    VkCommandBufferInheritanceInfo *secondaryCommandBufferInfo) {
  VkCommandBufferBeginInfo commandBufferBeginInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, usage,
      secondaryCommandBufferInfo};

  const VkResult result =
      vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
  if (VK_SUCCESS != result) {
    std::cout << "Could not begin command buffer recording operation."
              << std::endl;
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

bool endCommandBufferRecordingOperation(const VkCommandBuffer commandBuffer) {
  const VkResult result = vkEndCommandBuffer(commandBuffer);
  if (VK_SUCCESS != result) {
    std::cout << "Error occurred during command buffer recording." << std::endl;
    return false;
  }
  return true;
}
bool submitCommandBuffersToQueue(
    VkQueue queue, std::vector<WaitSemaphoreInfo> waitSemaphoreInfos,
    std::vector<VkCommandBuffer> commandBuffers,
    std::vector<VkSemaphore> signalSemaphores, VkFence& fence) {
  std::vector<VkSemaphore> waitSemaphoreHandles;
  std::vector<VkPipelineStageFlags> waitSemaphoreStages;

  for (auto &waitSemaphoreInfo : waitSemaphoreInfos) {
    waitSemaphoreHandles.emplace_back(waitSemaphoreInfo.semaphore);
    waitSemaphoreStages.emplace_back(waitSemaphoreInfo.waitingStage);
  }

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             nullptr,
                             static_cast<uint32_t>(waitSemaphoreInfos.size()),
                             waitSemaphoreHandles.data(),
                             waitSemaphoreStages.data(),
                             static_cast<uint32_t>(commandBuffers.size()),
                             commandBuffers.data(),
                             static_cast<uint32_t>(signalSemaphores.size()),
                             signalSemaphores.data()};

  const VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
  if (VK_SUCCESS != result) {
    std::cout << "Error occurred during command buffer submission."
              << std::endl;
    return false;
  }
  return true;
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

} // namespace vk
} // namespace SirEngine