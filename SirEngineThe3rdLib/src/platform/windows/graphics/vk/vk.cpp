
#define VOLK_IMPLEMENTATION
#include "volk.h"

#include "SirEngine/Window.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkAdapter.h"
#include "platform/windows/graphics/vk/vkLoad.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "platform/windows/graphics/vk/vkShaderManager.h"
#include "platform/windows/graphics/vk/vkSwapChain.h"
#include "vkDescriptors.h"
#include "vkRootSignatureManager.h"

namespace SirEngine::vk {
VkInstance INSTANCE = nullptr;
VkSurfaceKHR SURFACE = nullptr;
VkDevice LOGICAL_DEVICE = nullptr;
VkQueue GRAPHICS_QUEUE = nullptr;
VkQueue COMPUTE_QUEUE = nullptr;
VkQueue PRESENTATION_QUEUE = nullptr;
VkPhysicalDevice PHYSICAL_DEVICE = nullptr;
VkSwapchain *SWAP_CHAIN = nullptr;
VkDescriptorPool DESCRIPTOR_POOL = nullptr;
;
VkFormat IMAGE_FORMAT = VK_FORMAT_UNDEFINED;
VkPipelineLayout PIPELINE_LAYOUT = nullptr;
VkDebugReportCallbackEXT DEBUG_CALLBACK = nullptr;
VkDebugUtilsMessengerEXT DEBUG_CALLBACK2 = nullptr;

VkPSOManager *PSO_MANAGER = nullptr;
VkShaderManager *SHADER_MANAGER = nullptr;
VkPipelineLayoutManager *PIPELINE_LAYOUT_MANAGER = nullptr;
uint32_t SWAP_CHAIN_IMAGE_COUNT = 0;
VkFrameCommand FRAME_COMMAND[PREALLOCATED_SEMAPHORE_COUNT];

// TODO move this to manager
std::vector<VkDescriptorSetLayout> LAYOUTS_TO_DELETE;

bool vkInitializeGraphics(BaseWindow *wnd, const uint32_t width,
                          const uint32_t height) {
  VK_CHECK(volkInitialize());

  std::vector<char const *> instanceExtensions;
  if (!vk::createVulkanInstanceWithWsiExtensionsEnabled(
          instanceExtensions, "Vulkan Viewport", INSTANCE)) {
    return false;
  }

  volkLoadInstance(INSTANCE);

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
  const bool adapterFound = getBestAdapter(adapterConfig, adapterResult);
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
                  SWAP_CHAIN, *swapchain);
  SWAP_CHAIN = swapchain;

  assert(SWAP_CHAIN_IMAGE_COUNT != 0);
  assert(SWAP_CHAIN_IMAGE_COUNT <= PREALLOCATED_SEMAPHORE_COUNT);

  for (uint32_t i = 0; i < SWAP_CHAIN_IMAGE_COUNT; ++i) {

    if (!newSemaphore(LOGICAL_DEVICE, (FRAME_COMMAND->m_acquireSemaphore))) {
      assert(0 && "failed to create acquire image semaphore");
    }

    if (!newSemaphore(LOGICAL_DEVICE, FRAME_COMMAND->m_renderSemaphore)) {
      assert(0 && "failed to create render semaphore");
    }

    // Command buffers creation
    if (!createCommandPool(
            LOGICAL_DEVICE, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            adapterResult.m_graphicsQueueFamilyIndex, FRAME_COMMAND->m_commandAllocator)) {
      assert(0 && "could not create command pool");
    }
	if (!allocateCommandBuffer(LOGICAL_DEVICE, FRAME_COMMAND->m_commandAllocator,
                              VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                              FRAME_COMMAND->m_commandBuffer)) {
    assert(0);
	}
  }


  // if constexpr (!USE_PUSH) {
  vk::createDescriptorPool(vk::LOGICAL_DEVICE, {10000, 10000}, DESCRIPTOR_POOL);
  //}

  SHADER_MANAGER = new VkShaderManager();
  SHADER_MANAGER->init();
  SHADER_MANAGER->loadShadersInFolder(
      frameConcatenation(globals::ENGINE_CONFIG->m_dataSourcePath,
                         "/processed/shaders/VK/rasterization"));
  PIPELINE_LAYOUT_MANAGER = new VkPipelineLayoutManager();
  PIPELINE_LAYOUT_MANAGER->init();

  PSO_MANAGER = new VkPSOManager();
  PSO_MANAGER->init();

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

bool VkRenderingContext::newFrame() {

  // TODO this need to be removed
  waitForAllSubmittedCommandsToBeFinished(LOGICAL_DEVICE);

  if (!acquireSwapchainImage(LOGICAL_DEVICE, SWAP_CHAIN->swapchain,
                             FRAME_COMMAND[0].m_acquireSemaphore, VK_NULL_HANDLE,
                             globals::CURRENT_FRAME)) {
    return false;
  }
  if (!beginCommandBufferRecordingOperation(
          FRAME_COMMAND[0].m_commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
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

  setImageMemoryBarrier(FRAME_COMMAND[0].m_commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        {imageTransitionBeforeDrawing});
  return true;
}

bool VkRenderingContext::dispatchFrame() {

  const ImageTransition imageTransitionBeforePresent = {
      SWAP_CHAIN->images[globals::CURRENT_FRAME],
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      VK_IMAGE_ASPECT_COLOR_BIT};
  setImageMemoryBarrier(FRAME_COMMAND[0].m_commandBuffer,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        {imageTransitionBeforePresent});

  if (!endCommandBufferRecordingOperation(FRAME_COMMAND[0].m_commandBuffer)) {
    return false;
  }

  const WaitSemaphoreInfo waitSemaphoreInfo = {
      FRAME_COMMAND[0].m_acquireSemaphore, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
  if (!submitCommandBuffersToQueue(
          PRESENTATION_QUEUE, {waitSemaphoreInfo}, {FRAME_COMMAND[0].m_commandBuffer},
          {FRAME_COMMAND[0].m_renderSemaphore}, VK_NULL_HANDLE)) {
    return false;
  }

  const PresentInfo presentInfo = {SWAP_CHAIN->swapchain,
                                   globals::CURRENT_FRAME};
  bool res = presentImage(PRESENTATION_QUEUE, {FRAME_COMMAND[0].m_renderSemaphore},
                          {presentInfo});
  vkDeviceWaitIdle(LOGICAL_DEVICE);
  return res;
}

bool VkRenderingContext::resize(const uint32_t width, const uint32_t height) {
  auto *swapchain = new VkSwapchain;
  createSwapchain(LOGICAL_DEVICE, PHYSICAL_DEVICE, SURFACE, width, height,
                  SWAP_CHAIN, *swapchain);
  SWAP_CHAIN = swapchain;
  return true;
}

bool VkRenderingContext::stopGraphic() {
  vkDeviceWaitIdle(LOGICAL_DEVICE);
  return true;
}

bool VkRenderingContext::shutdownGraphic() {

  vkDeviceWaitIdle(LOGICAL_DEVICE);

  bool result = destroySwapchain(LOGICAL_DEVICE, SWAP_CHAIN);
  assert(result);

  destroyStaticSamplers();
  SHADER_MANAGER->cleanup();
  vkDestroyPipelineLayout(LOGICAL_DEVICE, PIPELINE_LAYOUT, nullptr);
  // vkDestroyRenderPass(LOGICAL_DEVICE, RENDER_PASS, nullptr);
  for (uint32_t i = 0; i < SWAP_CHAIN_IMAGE_COUNT; ++i) {
    vkDestroySemaphore(LOGICAL_DEVICE, FRAME_COMMAND[i].m_acquireSemaphore, nullptr);
    vkDestroySemaphore(LOGICAL_DEVICE, FRAME_COMMAND[i].m_renderSemaphore, nullptr);
	vkDestroyCommandPool(LOGICAL_DEVICE,FRAME_COMMAND[i].m_commandAllocator , nullptr);
  }
  vkDestroyDevice(LOGICAL_DEVICE, nullptr);
  vkDestroySurfaceKHR(INSTANCE, SURFACE, nullptr);
  // vkDestroyDebugReportCallbackEXT(INSTANCE, DEBUG_CALLBACK, nullptr);
  vkDestroyDebugUtilsMessengerEXT(INSTANCE, DEBUG_CALLBACK2, nullptr);
  vkDestroyInstance(INSTANCE, nullptr);
  return true;
}

void VkRenderingContext::flush() { vkDeviceWaitIdle(LOGICAL_DEVICE); }

void VkRenderingContext::executeGlobalCommandList() { assert(0); }

void VkRenderingContext::resetGlobalCommandList() { assert(0); }
} // namespace SirEngine::vk
