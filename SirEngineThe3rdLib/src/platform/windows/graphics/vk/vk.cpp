#include "VulkanFunctions.h"
#include "platform/windows/graphics/vk/vkLoad.h"
//#include "SirEngine/globals.h"
//#include "swapchain.h"
#include "platform/windows/graphics/vk/vk.h"
#include <cassert>

namespace vk {
VkInstance INSTANCE = nullptr;
VkSurfaceKHR SURFACE = nullptr;
static HMODULE VULKAN_LIBRARY = nullptr;
VkDevice LOGICAL_DEVICE = nullptr;
VkQueue GRAPHICS_QUEUE = nullptr;
VkQueue COMPUTE_QUEUE = nullptr;
VkQueue PRESENTATION_QUEUE = nullptr;
VkPhysicalDevice PHYSICAL_DEVICE = nullptr;
Swapchain *SWAP_CHAIN = nullptr;
VkRenderPass RENDER_PASS = nullptr;
VkSemaphore IMAGE_ACQUIRED_SEMAPHORE = nullptr;
VkSemaphore READY_TO_PRESENT_SEMAPHORE = nullptr;
VkCommandPool COMMAND_POOL = nullptr;
VkCommandBuffer COMMAND_BUFFER = nullptr;
VkFormat IMAGE_FORMAT = VK_FORMAT_UNDEFINED;
VkPipelineLayout PIPELINE_LAYOUT = nullptr;
VkDebugReportCallbackEXT DEBUG_CALLBACK = nullptr;
VkDebugUtilsMessengerEXT DEBUG_CALLBACK2 = nullptr;
std::vector<VkDescriptorSetLayout> LAYOUTS_TO_DELETE;

void initializeGraphics(const HINSTANCE hinstance, const HWND hwnd) {
  VULKAN_LIBRARY = LoadLibrary(L"vulkan-1.dll");
  assert(VULKAN_LIBRARY != nullptr);

  if (!vk::loadFunctionExportedFromVulkanLoaderLibrary(VULKAN_LIBRARY)) {
    return;
  }

  if (!vk::loadGlobalLevelFunctions()) {
    return;
  }

  std::vector<char const *> instanceExtensions;
  if (!vk::createVulkanInstanceWithWsiExtensionsEnabled(
          instanceExtensions, "Vulkan Viewport", INSTANCE)) {
    return;
  }

  if (!vk::loadInstanceLevelFunctions(INSTANCE, instanceExtensions)) {
    return;
  }

  vk::registerDebugCallback(INSTANCE);

  // init swap chain
  VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, 0, hinstance,
      hwnd};

  const VkResult result = vkCreateWin32SurfaceKHR(
      vk::INSTANCE, &surfaceCreateInfo, nullptr, &vk::SURFACE);

  assert(VK_SUCCESS == result);

  // Logical device creation
  std::vector<VkPhysicalDevice> physicalDevices;
  enumerateAvailablePhysicalDevices(INSTANCE, physicalDevices);

  uint32_t graphicsQueueFamilyIndex = 0;
  uint32_t presentQueueFamilyIndex;
  for (auto &physicalDevice : physicalDevices) {
    if (!selectIndexOfQueueFamilyWithDesiredCapabilities(
            physicalDevice, VK_QUEUE_GRAPHICS_BIT, graphicsQueueFamilyIndex)) {
      continue;
    }

    if (!selectQueueFamilyThatSupportsPresentationToGivenSurface(
            physicalDevice, SURFACE, presentQueueFamilyIndex)) {
      continue;
    }

    std::vector<QueueInfo> requestedQueues = {
        {graphicsQueueFamilyIndex, {1.0f}}};
    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
      requestedQueues.push_back({presentQueueFamilyIndex, {1.0f}});
    }
    std::vector<char const *> deviceExtensions;

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

    if (!createLogicalDeviceWithWsiExtensionsEnabled(
            physicalDevice, requestedQueues, deviceExtensions, &deviceFeatures,
            LOGICAL_DEVICE)) {
      continue;
    } else {
      if (!loadDeviceLevelFunctions(LOGICAL_DEVICE, deviceExtensions)) {
        continue;
      }
      PHYSICAL_DEVICE = physicalDevice;
      // LOGICAL_DEVICE= std::move(logical_device);
      getDeviceQueue(LOGICAL_DEVICE, graphicsQueueFamilyIndex, 0,
                     GRAPHICS_QUEUE);
      getDeviceQueue(LOGICAL_DEVICE, presentQueueFamilyIndex, 0,
                     PRESENTATION_QUEUE);
      break;
    }
  }

  assert(LOGICAL_DEVICE != nullptr);
  // create swap
  /*
  const auto swapchain = new Swapchain();
  createSwapchain(LOGICAL_DEVICE, PHYSICAL_DEVICE, SURFACE,
                  globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT, SWAP_CHAIN,
                  *swapchain, RENDER_PASS);
  SWAP_CHAIN = swapchain;

  if (!newSemaphore(LOGICAL_DEVICE, IMAGE_ACQUIRED_SEMAPHORE)) {
    assert(0);
  }

  if (!newSemaphore(LOGICAL_DEVICE, READY_TO_PRESENT_SEMAPHORE)) {
    assert(0);
  }

  // Command buffers creation
  if (!createCommandPool(LOGICAL_DEVICE,
                         VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                         graphicsQueueFamilyIndex, COMMAND_POOL)) {
    assert(0);
  }

  std::vector<VkCommandBuffer> commandBuffers;
  if (!allocateCommandBuffers(LOGICAL_DEVICE, COMMAND_POOL,
                              VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
                              commandBuffers)) {
    assert(0);
  }
  COMMAND_BUFFER = commandBuffers[0];
  */
}

bool acquireSwapchainImage(const VkDevice logicalDevice,
                           const VkSwapchainKHR swapchain,
                           const VkSemaphore semaphore, const VkFence fence,
                           uint32_t &imageIndex) {
  const VkResult result = vkAcquireNextImageKHR(
      logicalDevice, swapchain, 2000000000, semaphore, fence, &imageIndex);
  switch (result) {
  case VK_SUCCESS:
  case VK_SUBOPTIMAL_KHR:
    return true;
  default:
    return false;
  }
}
bool newFrame() {
  /*
waitForAllSubmittedCommandsToBeFinished(LOGICAL_DEVICE);

if (!acquireSwapchainImage(LOGICAL_DEVICE, SWAP_CHAIN->swapchain,
                       IMAGE_ACQUIRED_SEMAPHORE, VK_NULL_HANDLE,
                       globals::CURRENT_FRAME)) {
return false;
}
if (!beginCommandBufferRecordingOperation(
    COMMAND_BUFFER, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    nullptr)) {
return false;
}

const ImageTransition imageTransitionBeforeDrawing = {
SWAP_CHAIN->images[globals::CURRENT_FRAME],
0,
VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
VK_IMAGE_LAYOUT_UNDEFINED,
VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
VK_QUEUE_FAMILY_IGNORED, // used for cross queue sync
VK_QUEUE_FAMILY_IGNORED,
VK_IMAGE_ASPECT_COLOR_BIT}; // this wont work if you have depth buffers

setImageMemoryBarrier(COMMAND_BUFFER, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                  {imageTransitionBeforeDrawing});

                                          */
  return true;
}

bool nextFrame() {

  /*
const ImageTransition imageTransitionBeforePresent = {
SWAP_CHAIN->images[globals::CURRENT_FRAME],
VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
VK_ACCESS_MEMORY_READ_BIT,
VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
VK_QUEUE_FAMILY_IGNORED,
VK_QUEUE_FAMILY_IGNORED,
VK_IMAGE_ASPECT_COLOR_BIT};
setImageMemoryBarrier(COMMAND_BUFFER,
                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                  {imageTransitionBeforePresent});

if (!endCommandBufferRecordingOperation(COMMAND_BUFFER)) {
return false;
}

const WaitSemaphoreInfo waitSemaphoreInfo = {
IMAGE_ACQUIRED_SEMAPHORE, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
if (!submitCommandBuffersToQueue(
    PRESENTATION_QUEUE, {waitSemaphoreInfo}, {COMMAND_BUFFER},
    {READY_TO_PRESENT_SEMAPHORE}, VK_NULL_HANDLE)) {
return false;
}

const PresentInfo presentInfo = {SWAP_CHAIN->swapchain,
                             globals::CURRENT_FRAME};
bool res = presentImage(PRESENTATION_QUEUE, {READY_TO_PRESENT_SEMAPHORE},
                {presentInfo});
vkDeviceWaitIdle(LOGICAL_DEVICE);
return res;
*/
  return false;
}
bool stopGraphics() {
  vkDeviceWaitIdle(LOGICAL_DEVICE);
  return true;
}
bool shutdownGraphics() {
  /*
vkDeviceWaitIdle(LOGICAL_DEVICE);

assert(destroySwapchain(LOGICAL_DEVICE, SWAP_CHAIN));
vkDestroyPipelineLayout(LOGICAL_DEVICE, PIPELINE_LAYOUT, nullptr);
vkDestroyRenderPass(LOGICAL_DEVICE, RENDER_PASS, nullptr);
vkDestroySemaphore(LOGICAL_DEVICE, IMAGE_ACQUIRED_SEMAPHORE, nullptr);
vkDestroySemaphore(LOGICAL_DEVICE, READY_TO_PRESENT_SEMAPHORE, nullptr);
vkDestroyCommandPool(LOGICAL_DEVICE, COMMAND_POOL, nullptr);
vkDestroyDevice(LOGICAL_DEVICE, nullptr);
vkDestroySurfaceKHR(INSTANCE, SURFACE, nullptr);
//vkDestroyDebugReportCallbackEXT(INSTANCE, DEBUG_CALLBACK, nullptr);
vkDestroyDebugUtilsMessengerEXT(INSTANCE,DEBUG_CALLBACK2 , nullptr);
vkDestroyInstance(INSTANCE, nullptr);
*/
  return true;
}

bool onResize(uint32_t width, uint32_t height) {
  /*
Swapchain *swapchain = new Swapchain;
createSwapchain(LOGICAL_DEVICE, PHYSICAL_DEVICE, SURFACE, width, height,
            SWAP_CHAIN, *swapchain, RENDER_PASS);
SWAP_CHAIN = swapchain;
*/
  return true;
}

void setDebugNameImpl() {}
} // namespace vk
