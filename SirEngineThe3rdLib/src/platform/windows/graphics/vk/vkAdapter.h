#pragma once
#include "SirEngine/graphics/graphicsDefines.h"
#include <vulkan/vulkan.h>

namespace SirEngine::vk {


struct VkAdapterResult {
  VkPhysicalDevice m_physicalDevice;
  VkDevice m_device;
  uint32_t m_graphicsQueueFamilyIndex;
  uint32_t m_presentQueueFamilyIndex;
};

bool getBestAdapter(const AdapterRequestConfig &config,VkAdapterResult& adapterResult);
void logPhysicalDevice(VkPhysicalDevice physicalDevice);

} // namespace SirEngine::vk
