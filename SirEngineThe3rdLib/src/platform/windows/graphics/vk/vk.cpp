#include "platform/windows/graphics/vk/VulkanFunctions.h"
#include "platform/windows/graphics/vk/vkLoad.h"
//#include "SirEngine/globals.h"
#include "SirEngine/Window.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkSwapchain.h"
#include "vkAdapter.h"
#include <cassert>

namespace SirEngine::vk {
VkInstance INSTANCE = nullptr;
VkSurfaceKHR SURFACE = nullptr;
static HMODULE VULKAN_LIBRARY = nullptr;
VkDevice LOGICAL_DEVICE = nullptr;
VkQueue GRAPHICS_QUEUE = nullptr;
VkQueue COMPUTE_QUEUE = nullptr;
VkQueue PRESENTATION_QUEUE = nullptr;
VkPhysicalDevice PHYSICAL_DEVICE = nullptr;
VkSwapchain *SWAP_CHAIN = nullptr;
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

bool vkInitializeGraphics(BaseWindow *wnd, const uint32_t width,
                          const uint32_t height) {
  VULKAN_LIBRARY = LoadLibrary(L"vulkan-1.dll");
  assert(VULKAN_LIBRARY != nullptr);

  if (!vk::loadFunctionExportedFromVulkanLoaderLibrary(VULKAN_LIBRARY)) {
    return false;
  }

  if (!vk::loadGlobalLevelFunctions()) {
    return false;
  }

  std::vector<char const *> instanceExtensions;
  if (!vk::createVulkanInstanceWithWsiExtensionsEnabled(
          instanceExtensions, "Vulkan Viewport", INSTANCE)) {
    return false;
  }

  if (!vk::loadInstanceLevelFunctions(INSTANCE, instanceExtensions)) {
    return false;
  }

  vk::registerDebugCallback(INSTANCE);

  assert(sizeof(HWND) == 8);
  assert(sizeof(HINSTANCE) == 8);
  const NativeWindow *nativeWindow = wnd->getNativeWindow();
  HWND hwnd;
  memcpy(&hwnd, &nativeWindow->data2, sizeof(HWND));
  HINSTANCE hinstance;
  memcpy(&hinstance, &nativeWindow->data, sizeof(HINSTANCE));

  // init swap chain
  VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, 0, hinstance,
      hwnd};

  const VkResult result = vkCreateWin32SurfaceKHR(
      vk::INSTANCE, &surfaceCreateInfo, nullptr, &vk::SURFACE);

  assert(VK_SUCCESS == result);

  // new adapter code here
  AdapterRequestConfig adapterConfig{};
  adapterConfig.m_vendor = globals::ENGINE_CONFIG->m_adapterVendor;
  adapterConfig.m_vendorTolerant = globals::ENGINE_CONFIG->m_vendorTolerant;
  adapterConfig.m_genericRule = globals::ENGINE_CONFIG->m_adapterSelectionRule;

  VkAdapterResult adapterResult{};
  bool adapterFound = getBestAdapter(adapterConfig, adapterResult);
  assert(adapterFound);
  PHYSICAL_DEVICE = adapterResult.m_physicalDevice;
  LOGICAL_DEVICE = adapterResult.m_device;
  if (globals::ENGINE_CONFIG->m_verboseStartup) {
    logPhysicalDevice(PHYSICAL_DEVICE);
  }

  getDeviceQueue(LOGICAL_DEVICE, adapterResult.m_graphicsQueueFamilyIndex, 0,
                 GRAPHICS_QUEUE);
  getDeviceQueue(LOGICAL_DEVICE, adapterResult.m_presentQueueFamilyIndex, 0,
                 PRESENTATION_QUEUE);

  // create swap
  const auto swapchain = new VkSwapchain();
  createSwapchain(LOGICAL_DEVICE, PHYSICAL_DEVICE, SURFACE, width, height,
                  SWAP_CHAIN, *swapchain, RENDER_PASS);
  SWAP_CHAIN = swapchain;

  if (!newSemaphore(LOGICAL_DEVICE, IMAGE_ACQUIRED_SEMAPHORE)) {
    assert(0);
  }

  if (!newSemaphore(LOGICAL_DEVICE, READY_TO_PRESENT_SEMAPHORE)) {
    assert(0);
  }

  // Command buffers creation
  if (!createCommandPool(
          LOGICAL_DEVICE, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
          adapterResult.m_graphicsQueueFamilyIndex, COMMAND_POOL)) {
    assert(0);
  }

  std::vector<VkCommandBuffer> commandBuffers;
  if (!allocateCommandBuffers(LOGICAL_DEVICE, COMMAND_POOL,
                              VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
                              commandBuffers)) {
    assert(0);
  }
  COMMAND_BUFFER = commandBuffers[0];
  return true;
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
bool vkNewFrame() {
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

  return true;
}

bool vkNextFrame() {

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
}
bool vkStopGraphics() {
  vkDeviceWaitIdle(LOGICAL_DEVICE);
  return true;
}
bool vkShutdownGraphics() {
  vkDeviceWaitIdle(LOGICAL_DEVICE);

  assert(destroySwapchain(LOGICAL_DEVICE, SWAP_CHAIN));
  vkDestroyPipelineLayout(LOGICAL_DEVICE, PIPELINE_LAYOUT, nullptr);
  vkDestroyRenderPass(LOGICAL_DEVICE, RENDER_PASS, nullptr);
  vkDestroySemaphore(LOGICAL_DEVICE, IMAGE_ACQUIRED_SEMAPHORE, nullptr);
  vkDestroySemaphore(LOGICAL_DEVICE, READY_TO_PRESENT_SEMAPHORE, nullptr);
  vkDestroyCommandPool(LOGICAL_DEVICE, COMMAND_POOL, nullptr);
  vkDestroyDevice(LOGICAL_DEVICE, nullptr);
  vkDestroySurfaceKHR(INSTANCE, SURFACE, nullptr);
  // vkDestroyDebugReportCallbackEXT(INSTANCE, DEBUG_CALLBACK, nullptr);
  vkDestroyDebugUtilsMessengerEXT(INSTANCE, DEBUG_CALLBACK2, nullptr);
  vkDestroyInstance(INSTANCE, nullptr);
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

RenderingContext *
createVkRenderingContext(const RenderingContextCreationSettings &settings,
                         uint32_t width, uint32_t height) {
  return new VkRenderingContext(settings, width, height);
}

VkRenderingContext::VkRenderingContext(
    const RenderingContextCreationSettings &settings, uint32_t width,
    uint32_t height)
    : RenderingContext(settings, width, height) {
  SE_CORE_INFO("Initializing a Vulkan context");
}

void setDebugNameImpl() {}
bool VkRenderingContext::initializeGraphics() {

  const bool result = vkInitializeGraphics(
      m_settings.window, m_screenInfo.width, m_screenInfo.height);
  if (!result) {
    SE_CORE_ERROR("FATAL: could not initialize graphics");
  }
  return result;
}

bool VkRenderingContext::newFrame() { return vkNewFrame(); }

bool VkRenderingContext::dispatchFrame() { return vkNextFrame(); }

bool VkRenderingContext::resize(uint32_t width, uint32_t height) {
  assert(0);
  return false;
}

bool VkRenderingContext::stopGraphic() { return vkStopGraphics(); }

bool VkRenderingContext::shutdownGraphic() { return vkShutdownGraphics(); }

void VkRenderingContext::flush() { assert(0); }

void VkRenderingContext::executeGlobalCommandList() { assert(0); }

void VkRenderingContext::resetGlobalCommandList() { assert(0); }
} // namespace SirEngine::vk
