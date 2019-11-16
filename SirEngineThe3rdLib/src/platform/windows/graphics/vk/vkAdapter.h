#pragma once
#include "SirEngine/graphics/graphicsDefines.h"
#include <vulkan/vulkan.h>

namespace SirEngine::vk {

struct AdapterRequestConfig {
  ADAPTER_VENDOR m_vendor;
  ADAPTER_SELECTION_RULE genericRule;
  bool vendorTolerant;
};

struct AdapterResult {
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  uint32_t graphicsQueueFamilyIndex;
  uint32_t presentQueueFamilyIndex;
};

bool getBestAdapter(const AdapterRequestConfig &config,AdapterResult& adapterResult);
void logPhysicalDevice(VkPhysicalDevice physicalDevice);

} // namespace SirEngine::vk
