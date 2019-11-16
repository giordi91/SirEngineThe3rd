#pragma once
#include <vulkan/vulkan.h>

#include <Windows.h>
#include <cassert>
#include <vector>

namespace vk {

struct Swapchain;

// runtime instances
extern VkInstance INSTANCE;
extern VkSurfaceKHR SURFACE;
extern VkDevice LOGICAL_DEVICE;
extern VkQueue GRAPHICS_QUEUE;
extern VkQueue COMPUTE_QUEUE;
extern VkPhysicalDevice PHYSICAL_DEVICE;
extern Swapchain *SWAP_CHAIN;
extern VkRenderPass RENDER_PASS;
extern VkSemaphore IMAGE_ACQUIRED_SEMAPHORE;
extern VkSemaphore READY_TO_PRESENT_SEMAPHORE;
extern VkCommandPool COMMAND_POOL;
extern VkCommandBuffer COMMAND_BUFFER;
extern VkFormat IMAGE_FORMAT;
extern VkPipelineLayout PIPELINE_LAYOUT;
extern VkDebugReportCallbackEXT DEBUG_CALLBACK;
extern VkDebugUtilsMessengerEXT DEBUG_CALLBACK2; 

extern std::vector<VkDescriptorSetLayout> LAYOUTS_TO_DELETE;

void initializeGraphics(HINSTANCE hinstance, HWND hwnd);
bool newFrame();
bool nextFrame();
bool stopGraphics();
bool shutdownGraphics();
bool onResize(uint32_t width, uint32_t height);

#define VK_CHECK(call)                                                         \
  do {                                                                         \
    VkResult result_ = call;                                                   \
    assert(result_ == VK_SUCCESS);                                             \
  } while (0)

#ifndef ARRAYSIZE
#define ARRAYSIZE(array) sizeof(array) / sizeof(array[0]);
#endif

//#if _DEBUG #endif                                                            \
//  }

#define SET_DEBUG_NAME(resource, type, name)                                   \
  {                                                                            \
    VkDebugUtilsObjectNameInfoEXT debugInfo_{};                                \
    debugInfo_.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;     \
    debugInfo_.objectHandle = (uint64_t)resource;                              \
    debugInfo_.objectType = type;                                              \
    debugInfo_.pObjectName = name;                                             \
    vkSetDebugUtilsObjectNameEXT(LOGICAL_DEVICE, &debugInfo_);                 \
  }

} // namespace vk
