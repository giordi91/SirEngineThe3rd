
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
VkFrameCommand *CURRENT_FRAME_COMMAND = nullptr;
uint32_t GRAPHICS_QUEUE_FAMILY = 0;
uint32_t PRESENTATION_QUEUE_FAMILY = 0;

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
  adapterConfig.m_vendor = globals::ENGINE_CONFIG->m_requestedAdapterVendor;
  adapterConfig.m_vendorTolerant = globals::ENGINE_CONFIG->m_vendorTolerant;
  adapterConfig.m_genericRule = globals::ENGINE_CONFIG->m_adapterSelectionRule;

  VkAdapterResult adapterResult{};
  const bool adapterFound = getBestAdapter(adapterConfig, adapterResult);
  assert(adapterFound);
  PHYSICAL_DEVICE = adapterResult.m_physicalDevice;
  LOGICAL_DEVICE = adapterResult.m_device;
  globals::ENGINE_CONFIG->m_selectdedAdapterVendor = adapterResult.m_foundVendor;
  if (globals::ENGINE_CONFIG->m_verboseStartup) {
    logPhysicalDevice(PHYSICAL_DEVICE);
  }

  GRAPHICS_QUEUE_FAMILY = adapterResult.m_graphicsQueueFamilyIndex;
  PRESENTATION_QUEUE_FAMILY = adapterResult.m_presentQueueFamilyIndex;

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

    if (!newSemaphore(LOGICAL_DEVICE, (FRAME_COMMAND[i].m_acquireSemaphore))) {
      assert(0 && "failed to create acquire image semaphore");
    }

    if (!newSemaphore(LOGICAL_DEVICE, FRAME_COMMAND[i].m_renderSemaphore)) {
      assert(0 && "failed to create render semaphore");
    }

    // Command buffers creation
    if (!createCommandPool(LOGICAL_DEVICE,
                           VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                           adapterResult.m_graphicsQueueFamilyIndex,
                           FRAME_COMMAND[i].m_commandAllocator)) {
      assert(0 && "could not create command pool");
    }
      SET_DEBUG_NAME(FRAME_COMMAND[i].m_commandAllocator,
                     VK_OBJECT_TYPE_COMMAND_POOL,
                     frameConcatenation("commandPool", std::to_string(i).c_str()));
    if (!allocateCommandBuffer(LOGICAL_DEVICE,
                               FRAME_COMMAND[i].m_commandAllocator,
                               VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                               FRAME_COMMAND[i].m_commandBuffer)) {
      SET_DEBUG_NAME(FRAME_COMMAND[i].m_commandBuffer,
                     VK_OBJECT_TYPE_COMMAND_BUFFER,
                     frameConcatenation("commandBuffer", std::to_string(i).c_str()));
      assert(0);
    }
  }

  CURRENT_FRAME_COMMAND = &FRAME_COMMAND[0];

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

void waitOnFence(VkFence fence) {
  if (fence == nullptr) {
    return;
  }
  VK_CHECK(vkWaitForFences(vk::LOGICAL_DEVICE, 1, &fence, true,
                           200000000));
}

void resetFrameCommand(VkFrameCommand *command) {
  vkResetCommandPool(LOGICAL_DEVICE, command->m_commandAllocator, 0);
  if (command->m_endOfFrameFence == nullptr) {
    VkFenceCreateInfo create_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VK_CHECK(vkCreateFence(LOGICAL_DEVICE, &create_info, nullptr,
                           &command->m_endOfFrameFence));
  }
  vkResetFences(LOGICAL_DEVICE, 1, &command->m_endOfFrameFence);
}

bool VkRenderingContext::newFrame() {

  // resetting memory used on a per frame basis
  globals::STRING_POOL->resetFrameMemory();
  globals::FRAME_ALLOCATOR->reset();

  // updating current frame command
  CURRENT_FRAME_COMMAND = &FRAME_COMMAND[globals::CURRENT_FRAME];
  assert(CURRENT_FRAME_COMMAND != nullptr);

  // now we need to check the fence
  waitOnFence(CURRENT_FRAME_COMMAND->m_endOfFrameFence);

  // we are good to go know, we know that the fence has been cleared and this
  // resources are not been used anymore
  resetFrameCommand(CURRENT_FRAME_COMMAND);

  // we can now acquire the image
  if (!acquireSwapchainImage(LOGICAL_DEVICE, SWAP_CHAIN->swapchain,
                             CURRENT_FRAME_COMMAND->m_acquireSemaphore,
                             VK_NULL_HANDLE, globals::CURRENT_FRAME)) {
    return false;
  }
  if (!beginCommandBufferRecordingOperation(
          CURRENT_FRAME_COMMAND->m_commandBuffer,
          VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr)) {
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

  setImageMemoryBarrier(CURRENT_FRAME_COMMAND->m_commandBuffer,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        {imageTransitionBeforeDrawing});
  return true;
}

bool VkRenderingContext::dispatchFrame() {

  assert(CURRENT_FRAME_COMMAND != nullptr);

  const ImageTransition imageTransitionBeforePresent = {
      SWAP_CHAIN->images[globals::CURRENT_FRAME],
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      VK_IMAGE_ASPECT_COLOR_BIT};
  setImageMemoryBarrier(CURRENT_FRAME_COMMAND->m_commandBuffer,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        {imageTransitionBeforePresent});

  if (!endCommandBufferRecordingOperation(
          CURRENT_FRAME_COMMAND->m_commandBuffer)) {
    return false;
  }

  const WaitSemaphoreInfo waitSemaphoreInfo = {
      CURRENT_FRAME_COMMAND->m_acquireSemaphore,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
  if (!submitCommandBuffersToQueue(PRESENTATION_QUEUE, {waitSemaphoreInfo},
                                   {CURRENT_FRAME_COMMAND->m_commandBuffer},
                                   {CURRENT_FRAME_COMMAND->m_renderSemaphore},
                                   CURRENT_FRAME_COMMAND->m_endOfFrameFence)) {
    return false;
  }

  const PresentInfo presentInfo = {SWAP_CHAIN->swapchain,
                                   globals::CURRENT_FRAME};
  bool res =
      presentImage(PRESENTATION_QUEUE,
                   {CURRENT_FRAME_COMMAND->m_renderSemaphore}, {presentInfo});
  // vkDeviceWaitIdle(LOGICAL_DEVICE);

  // total number of frames is updated at the beginning of the frame by the
  // application
  globals::CURRENT_FRAME =
      (globals::TOTAL_NUMBER_OF_FRAMES) % SWAP_CHAIN_IMAGE_COUNT;

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
    vkDestroySemaphore(LOGICAL_DEVICE, FRAME_COMMAND[i].m_acquireSemaphore,
                       nullptr);
    vkDestroySemaphore(LOGICAL_DEVICE, FRAME_COMMAND[i].m_renderSemaphore,
                       nullptr);
    vkDestroyCommandPool(LOGICAL_DEVICE, FRAME_COMMAND[i].m_commandAllocator,
                         nullptr);
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
