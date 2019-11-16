
#include "platform/windows/graphics/vk/vkAdapter.h"
#include "SirEngine/log.h"
#include "vk.h"
#include "vkLoad.h"
#include <cassert>
#include <iostream>
#include <vector>

namespace SirEngine::vk {

uint32_t VENDOR_ID[] = {0x10DE, 0x1002, 0x8086, 0xFFFF, 0xFFFF};

bool physicalDeviceSupportsQueues(VkPhysicalDevice physicalDevice,
                                  uint32_t &graphicsQueueFamilyIndex,
                                  uint32_t &presentQueueFamilyIndex) {
  // the device needs to be able to do graphics
  if (!selectIndexOfQueueFamilyWithDesiredCapabilities(
          physicalDevice, VK_QUEUE_GRAPHICS_BIT, graphicsQueueFamilyIndex)) {
    return false;
  }

  // and also being able to present to the given surface
  if (!selectQueueFamilyThatSupportsPresentationToGivenSurface(
          physicalDevice, SURFACE, presentQueueFamilyIndex)) {
    return false;
  }
  return true;
}

bool createLogicalDevice(VkPhysicalDevice physicalDevice,
                         AdapterResult &adapterResult) {
  // if we got to this point, it means we have the necessary queues
  // indices
  std::vector<QueueInfo> requestedQueues = {
      {adapterResult.graphicsQueueFamilyIndex, {1.0f}}};
  if (adapterResult.graphicsQueueFamilyIndex !=
      adapterResult.presentQueueFamilyIndex) {
    requestedQueues.push_back({adapterResult.presentQueueFamilyIndex, {1.0f}});
  }
  std::vector<char const *> deviceExtensions;

  // for now such features are so standard that we won't worry about exposing
  // them higher up, from settings for examples
  VkPhysicalDevice8BitStorageFeaturesKHR feature8{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR};
  feature8.storageBuffer8BitAccess = true;
  feature8.uniformAndStorageBuffer8BitAccess = true;

  VkPhysicalDeviceFeatures2 deviceFeatures{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
  deviceFeatures.features = {};
  deviceFeatures.features.vertexPipelineStoresAndAtomics = true;
  deviceFeatures.features.geometryShader = true;

  deviceFeatures.pNext = &feature8;

  // lets create a logical queue with the requested features
  if (!createLogicalDeviceWithWsiExtensionsEnabled(
          physicalDevice, requestedQueues, deviceExtensions, &deviceFeatures,
          adapterResult.device)) {
    return false;
  } else {
    if (!loadDeviceLevelFunctions(adapterResult.device, deviceExtensions)) {
      return false;
    }
    adapterResult.physicalDevice = physicalDevice;
  }
}

uint32_t getMaxPhysicalDeviceVRAMSizeInGB(VkPhysicalDevice physicalDevice) {

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

  uint32_t gb = 0;
  for (unsigned int i = 0; i < memoryProperties.memoryHeapCount; ++i) {
    for (unsigned int h = 0; h < memoryProperties.memoryTypeCount; ++h) {
      if (memoryProperties.memoryTypes[h].heapIndex == i) {
        if (memoryProperties.memoryTypes[h].propertyFlags &
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
          uint32_t tempGB = memoryProperties.memoryHeaps[i].size * 1.0e-9;
          gb = gb < tempGB ? tempGB : gb;
        }
      }
    }
  }
  return gb;
}

bool findSpecificVendorBestAdapter(
    const AdapterRequestConfig &config,
    const std::vector<VkPhysicalDevice> &physicalDevices,
    AdapterResult &adapterResult) {

  // it means we have a preference for an hardware vendor so let us try look
  // for it
  uint32_t largestFrameBuffer = 0;
  VkPhysicalDeviceProperties properties;
  for (auto &physicalDevice : physicalDevices) {
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    if (properties.vendorID !=
        VENDOR_ID[static_cast<uint32_t>(config.m_vendor)]) {
      continue;
    }
    // we found correct vendor, we need to see if support what we need
    bool result = physicalDeviceSupportsQueues(
        physicalDevice, adapterResult.graphicsQueueFamilyIndex,
        adapterResult.presentQueueFamilyIndex);
    if (!result) {
      continue;
    }
    uint32_t currentGbSize = getMaxPhysicalDeviceVRAMSizeInGB(physicalDevice);

    // if our rule is to use the largest frame buffer we check
    // the current size, if it is smaller than the one already provided,
    // there is no point in going further and we just go the the next
    if ((config.genericRule == ADAPTER_SELECTION_RULE::LARGEST_FRAME_BUFFER) &
        (currentGbSize < largestFrameBuffer)) {
      continue;
    }

    // if we are here means our device supports required queues, lets
    // create the logical device
    result = createLogicalDevice(physicalDevice, adapterResult);

    // if it failed it means most likely does not support the basic
    // features we requested like stores atomics, geo shaders and 8 bit
    // storage
    if (result) {
      // we update the largest frame buffer due to success, if the rule is
      // first valid we return anyway
      largestFrameBuffer = largestFrameBuffer < currentGbSize
                               ? currentGbSize
                               : largestFrameBuffer;

      if (config.genericRule == ADAPTER_SELECTION_RULE::FIRST_VALID) {
        return true;
      }
    }
  }
  return largestFrameBuffer != 0;
}

bool getBestAdapter(const AdapterRequestConfig &config,
                    AdapterResult &adapterResult) {
  // Logical device creation
  std::vector<VkPhysicalDevice> physicalDevices;
  enumerateAvailablePhysicalDevices(INSTANCE, physicalDevices);

  if (config.m_vendor != ADAPTER_VENDOR::ANY) {
    bool result =
        findSpecificVendorBestAdapter(config, physicalDevices, adapterResult);
	//if found we are all good, otherwise we might have to do some error handling
    if (result) {
      return true;
    }

    if (!result && !config.vendorTolerant) {
      // it means we found no valid adapter and we cannot search for fallback
      SE_CORE_ERROR("Could not find requested vendor {0}, and fail back to any "
                    "vendor is not allowed by settings",
                    ADAPTER_VENDOR_NAMES[static_cast<int>(config.m_vendor)]);
      exit(EXIT_FAILURE);
    }
    SE_CORE_WARN(
        "Could not find requesed vendor adapter {0}, going for fallback",
        ADAPTER_VENDOR_NAMES[static_cast<int>(config.m_vendor)]);
  }

  uint32_t largestFrameBuffer = 0;
  // if we are here is because we want any type of adapter
  // we iterate all the devices
  for (auto &physicalDevice : physicalDevices) {

    // we found correct vendor, we need to see if support what we need
    bool result = physicalDeviceSupportsQueues(
        physicalDevice, adapterResult.graphicsQueueFamilyIndex,
        adapterResult.presentQueueFamilyIndex);
    if (!result) {
      continue;
    }

    uint32_t currentGbSize = getMaxPhysicalDeviceVRAMSizeInGB(physicalDevice);

    // if our rule is to use the largest frame buffer we check
    // the current size, if it is smaller than the one already provided,
    // there is no point in going further and we just go the the next
    if ((config.genericRule == ADAPTER_SELECTION_RULE::LARGEST_FRAME_BUFFER) &
        (currentGbSize < largestFrameBuffer)) {
      continue;
    }

    // if we are here means our device supports required queues, lets
    // create the logical device
    result = createLogicalDevice(physicalDevice, adapterResult);

    // if it failed it means most likely does not support the basic
    // features we requested like stores atomics, geo shaders and 8 bit
    // storage
    if (result) {
      // we update the largest frame buffer due to success, if the rule is
      // first valid we return anyway
      largestFrameBuffer = largestFrameBuffer < currentGbSize
                               ? currentGbSize
                               : largestFrameBuffer;

      if (config.genericRule == ADAPTER_SELECTION_RULE::FIRST_VALID) {
        return true;
      }
    }
  }

  assert(adapterResult.device != nullptr);
  assert(adapterResult.physicalDevice != nullptr);

  return largestFrameBuffer != 0;
}

void logPhysicalDevice(VkPhysicalDevice physicalDevice) {

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
}
} // namespace SirEngine::vk
