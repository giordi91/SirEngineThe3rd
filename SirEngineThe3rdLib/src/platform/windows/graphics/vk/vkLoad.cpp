
#include "platform/windows/graphics/vk/vkLoad.h"
#include "platform/windows/graphics/vk/VulkanFunctions.h"
#include "vk.h"
#include <cassert>
#include <iostream>

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

bool loadFunctionExportedFromVulkanLoaderLibrary(
    LIBRARY_TYPE const &vulkanLibrary) {
#if defined _WIN32
#define LoadFunction GetProcAddress
#elif defined __linux
#define LoadFunction dlsym
#endif

#define EXPORTED_VULKAN_FUNCTION(name)                                         \
  name = (PFN_##name)LoadFunction(vulkanLibrary, #name);                       \
  if (name == nullptr) {                                                       \
    std::cout << "Could not load exported Vulkan function named: " #name       \
              << std::endl;                                                    \
    return false;                                                              \
  }

#include "ListOfVulkanFunctions.inl"

  return true;
}

bool loadGlobalLevelFunctions() {
#define GLOBAL_LEVEL_VULKAN_FUNCTION(name)                                     \
  name = (PFN_##name)vkGetInstanceProcAddr(nullptr, #name);                    \
  if (name == nullptr) {                                                       \
    std::cout << "Could not load global level Vulkan function named: " #name   \
              << std::endl;                                                    \
    return false;                                                              \
  }

#include "ListOfVulkanFunctions.inl"

  return true;
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
  VkResult result = VK_SUCCESS;

  result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount,
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
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
  debug_utils_messenger_create_info.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  debug_utils_messenger_create_info.pfnUserCallback = debugCallback2;
  if (VK_SUCCESS != vkCreateDebugUtilsMessengerEXT(
                        instance, &debug_utils_messenger_create_info, NULL,
                        &DEBUG_CALLBACK2))
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

bool loadInstanceLevelFunctions(VkInstance instance,
                                std::vector<char const *> const &extensions) {
  // Load core Vulkan API instance-level functions
#define INSTANCE_LEVEL_VULKAN_FUNCTION(name)                                   \
  name = (PFN_##name)vkGetInstanceProcAddr(instance, #name);                   \
  if (name == nullptr) {                                                       \
    std::cout << "Could not load instance-level Vulkan function named: " #name \
              << std::endl;                                                    \
    return false;                                                              \
  }

  // Load instance-level functions from enabled extensions
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)         \
  for (auto &enabled_extension : extensions) {                                 \
    if (std::string(enabled_extension) == std::string(extension)) {            \
      name = (PFN_##name)vkGetInstanceProcAddr(instance, #name);               \
      if (name == nullptr) {                                                   \
        std::cout                                                              \
            << "Could not load instance-level Vulkan function named: " #name   \
            << std::endl;                                                      \
        return false;                                                          \
      }                                                                        \
    }                                                                          \
  }

#include "ListOfVulkanFunctions.inl"

  return true;
}

bool loadDeviceLevelFunctions(
    VkDevice logicalDevice,
    std::vector<char const *> const &enabledExtensions) {
  // Load core Vulkan API device-level functions
#define DEVICE_LEVEL_VULKAN_FUNCTION(name)                                     \
  name = (PFN_##name)vkGetDeviceProcAddr(logicalDevice, #name);                \
  if (name == nullptr) {                                                       \
    std::cout << "Could not load device-level Vulkan function named: " #name   \
              << std::endl;                                                    \
    return false;                                                              \
  }

  // Load device-level functions from enabled extensions
#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)           \
  for (auto &enabledExtension : enabledExtensions) {                           \
    if (std::string(enabledExtension) == std::string(extension)) {             \
      name = (PFN_##name)vkGetDeviceProcAddr(logicalDevice, #name);            \
      if (name == nullptr) {                                                   \
        std::cout                                                              \
            << "Could not load device-level Vulkan function named: " #name     \
            << std::endl;                                                      \
        return false;                                                          \
      }                                                                        \
    }                                                                          \
  }

#include "ListOfVulkanFunctions.inl"

  return true;
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
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
  std::cout << "Physical device used:\n"
            << properties.deviceName
            << "\nDriver version: " << properties.driverVersion << "\nVRAM: ";
  for (unsigned int i = 0; i < memoryProperties.memoryHeapCount; ++i) {
    std::cout << "\n    Heap size:"
              << memoryProperties.memoryHeaps[i].size * 1.0e-9 << " GB\n"
              << "    Heap type: \n";
    for (unsigned int h = 0; h < memoryProperties.memoryTypeCount; ++h) {
      if (memoryProperties.memoryTypes[h].heapIndex == i) {
        if (memoryProperties.memoryTypes[h].propertyFlags &
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
          std::cout << "        DEVICE_LOCAL\n";
        }
        if (memoryProperties.memoryTypes[h].propertyFlags &
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
          std::cout << "        HOST_VISIBLE\n";
        }
        if (memoryProperties.memoryTypes[h].propertyFlags &
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
          std::cout << "        HOST_COHERENT\n";
        }
        if (memoryProperties.memoryTypes[h].propertyFlags &
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
          std::cout << "        HOST_CACHED\n";
        }
        if (memoryProperties.memoryTypes[h].propertyFlags &
            VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
          std::cout << "        LAZY_ALLOCATED\n";
        }
        if (memoryProperties.memoryTypes[h].propertyFlags &
            VK_MEMORY_PROPERTY_PROTECTED_BIT) {
          std::cout << "        PROTECTED_BIT\n";
        }
      }
    }
  }
  std::cout << std::endl;

  return true;
}

void getDeviceQueue(const VkDevice logicalDevice,
                    const uint32_t queueFamilyIndex, const uint32_t queueIndex,
                    VkQueue &queue) {
  vkGetDeviceQueue(logicalDevice, queueFamilyIndex, queueIndex, &queue);
}

/*
bool createLogicalDeviceWithGeometryShadersAndGraphicsAndComputeQueues(
    VkInstance instance, VkDevice &logicalDevice, VkQueue &graphicsQueue,
    VkQueue &computeQueue) {
  std::vector<VkPhysicalDevice> physicalDevices;
  enumerateAvailablePhysicalDevices(instance, physicalDevices);

  // after we enumerated the physical devices we are going to query the
  // different feature
  for (const auto &physicalDevice : physicalDevices) {
    VkPhysicalDeviceProperties deviceProperties;
    getFeaturesAndPropertiesOfPhysicalDevice(physicalDevice, deviceFeatures,
                                             deviceProperties);

    // if (!deviceFeatures.geometryShader) {
    //  continue;
    //} else {
    //  deviceFeatures = {};
    //  deviceFeatures.geometryShader = VK_TRUE;
    //}


    uint32_t graphicsQueueFamilyIndex;
    if (!selectIndexOfQueueFamilyWithDesiredCapabilities(
            physicalDevice, VK_QUEUE_GRAPHICS_BIT, graphicsQueueFamilyIndex)) {
      continue;
    }

    uint32_t computeQueueFamilyIndex;
    if (!selectIndexOfQueueFamilyWithDesiredCapabilities(
            physicalDevice, VK_QUEUE_COMPUTE_BIT, computeQueueFamilyIndex)) {
      continue;
    }

    // now we found index for both queues we needed, if they have the same index
    // we make a single request
    std::vector<QueueInfo> requestedQueues = {
        {graphicsQueueFamilyIndex, {1.0f}}};
    if (graphicsQueueFamilyIndex != computeQueueFamilyIndex) {
      requestedQueues.push_back({computeQueueFamilyIndex, {1.0f}});
    }

    if (!createLogicalDevice(physicalDevice, requestedQueues, {},
                             &deviceFeatures, logicalDevice)) {
      continue;
    } else {
      if (!loadDeviceLevelFunctions(logicalDevice, {})) {
        return false;
      }
      getDeviceQueue(logicalDevice, graphicsQueueFamilyIndex, 0, graphicsQueue);
      getDeviceQueue(logicalDevice, computeQueueFamilyIndex, 0, computeQueue);

      return true;
    }
  }
  return false;
}
*/
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
  desiredExtensions.emplace_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
  desiredExtensions.emplace_back(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
  desiredExtensions.emplace_back(VK_KHR_8BIT_STORAGE_EXTENSION_NAME);

  return createLogicalDevice(physicalDevice, queueInfos, desiredExtensions,
                             desiredFeatures, logicalDevice);
}
bool waitForAllSubmittedCommandsToBeFinished(const VkDevice logicalDevice) {
  const VkResult result = vkDeviceWaitIdle(logicalDevice);
  if (VK_SUCCESS != result) {
    std::cout << "Waiting on a device failed." << std::endl;
    return false;
  }
  return true;
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
    std::cout << "Could not create command pool." << std::endl;
    return false;
  }
  return true;
}
bool allocateCommandBuffers(const VkDevice logicalDevice,
                            const VkCommandPool commandPool,
                            const VkCommandBufferLevel level,
                            const uint32_t count,
                            std::vector<VkCommandBuffer> &commandBuffers) {
  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, commandPool,
      level, count};

  commandBuffers.resize(count);

  const VkResult result = vkAllocateCommandBuffers(
      logicalDevice, &commandBufferAllocateInfo, commandBuffers.data());
  if (VK_SUCCESS != result) {
    std::cout << "Could not allocate command buffers." << std::endl;
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
    std::vector<VkSemaphore> signalSemaphores, const VkFence fence) {
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

VkRenderPass createRenderPass(VkDevice logicalDevice) {
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

  vkCreateRenderPass(logicalDevice, &createInfo, nullptr, &renderPass);
  return renderPass;
}

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

VkShaderModule loadShader(const VkDevice logicalDevice, const char *path) {
  FILE *file = fopen(path, "rb");
  assert(file);

  fseek(file, 0, SEEK_END);
  size_t length = ftell(file);
  assert(length > 0);

  fseek(file, 0, SEEK_SET);
  char *buffer = new char[length];
  size_t rc = fread(buffer, sizeof(char), length, file);
  assert(rc == length);

  fclose(file);

  assert(length % 4 == 0);
  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  createInfo.codeSize = length;
  createInfo.pCode = reinterpret_cast<uint32_t *>(buffer);

  VkShaderModule shaderModule = nullptr;
  vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule);
  delete[] buffer;
  return shaderModule;
}

VkPipeline
createGraphicsPipeline(VkDevice logicalDevice, VkShaderModule vs,
                       VkShaderModule ps, VkRenderPass renderPass,
                       VkPipelineVertexInputStateCreateInfo *vertexInfo) {
  VkPipelineLayoutCreateInfo layoutInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

  VkDescriptorSetLayoutBinding bindings[2]{};
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo descriptorInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  //descriptorInfo.flags =
  //    VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;

  descriptorInfo.bindingCount = ARRAYSIZE(bindings);
  descriptorInfo.pBindings = bindings;

  VkDescriptorSetLayout descriptorLayout;
  vkCreateDescriptorSetLayout(logicalDevice, &descriptorInfo, nullptr,
                              &descriptorLayout);

  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &descriptorLayout;

  vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &PIPELINE_LAYOUT);

  LAYOUTS_TO_DELETE.push_back(descriptorLayout);

  VkPipelineShaderStageCreateInfo stages[2] = {};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vs;
  stages[0].pName = "main";
  // this allows us to change constants at pipeline creation time,
  // this can allow for example to change compute shaders group size
  // and possibly allow brute force benchmarks with different sizes
  // vsInfo.pSpecializationInfo;
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = ps;
  stages[1].pName = "main";

  VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyCreateInfo.primitiveRestartEnable = false;

  VkPipelineViewportStateCreateInfo viewportCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewportCreateInfo.viewportCount = 1;
  viewportCreateInfo.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterInfo{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterInfo.depthClampEnable = false;
  rasterInfo.rasterizerDiscardEnable = false;
  rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
  rasterInfo.lineWidth = 1.0f; // even if we don't use it must be specified
  rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  ;

  VkPipelineMultisampleStateCreateInfo multisampleState{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkGraphicsPipelineCreateInfo createInfo{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  VkPipelineDepthStencilStateCreateInfo depthStencilState = {
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  depthStencilState.depthTestEnable = false;

  VkPipelineColorBlendAttachmentState attachState{};
  attachState.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo blendState{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  blendState.attachmentCount = 1;
  blendState.pAttachments = &attachState;

  VkDynamicState dynStateFlags[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = {
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamicState.dynamicStateCount =
      sizeof(dynStateFlags) / sizeof(dynStateFlags[0]);
  dynamicState.pDynamicStates = dynStateFlags;

  createInfo.stageCount = 2;
  createInfo.pStages = stages;
  createInfo.pVertexInputState =
      vertexInfo != nullptr ? vertexInfo : &vertexInputCreateInfo;
  createInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
  createInfo.pViewportState = &viewportCreateInfo;
  createInfo.pRasterizationState = &rasterInfo;
  createInfo.pMultisampleState = &multisampleState;
  createInfo.pDepthStencilState = &depthStencilState;
  createInfo.pColorBlendState = &blendState;
  createInfo.pDynamicState = &dynamicState;
  createInfo.renderPass = renderPass;
  createInfo.layout = PIPELINE_LAYOUT;

  VkPipeline pipeline = nullptr;
  VkResult status = vkCreateGraphicsPipelines(logicalDevice, nullptr, 1,
                                              &createInfo, nullptr, &pipeline);
  assert(status == VK_SUCCESS);
  assert(pipeline);
  return pipeline;
}
} // namespace vk