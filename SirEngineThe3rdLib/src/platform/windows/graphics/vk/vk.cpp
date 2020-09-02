
#define VOLK_IMPLEMENTATION
#include "platform/windows/graphics/vk/vk.h"

#include "SirEngine/Window.h"
#include "SirEngine/assetManager.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/camera.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/graphics/debugRenderer.h"
#include "SirEngine/graphics/lightManager.h"
#include "SirEngine/interopData.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"
#include "platform/windows/graphics/vk/vkAdapter.h"
#include "platform/windows/graphics/vk/vkBindingTableManager.h"
#include "platform/windows/graphics/vk/vkBufferManager.h"
#include "platform/windows/graphics/vk/vkConstantBufferManager.h"
#include "platform/windows/graphics/vk/vkLoad.h"
#include "platform/windows/graphics/vk/vkMaterialManager.h"
#include "platform/windows/graphics/vk/vkMeshManager.h"
#include "platform/windows/graphics/vk/vkPSOManager.h"
#include "platform/windows/graphics/vk/vkRootSignatureManager.h"
#include "platform/windows/graphics/vk/vkShaderManager.h"
#include "platform/windows/graphics/vk/vkSwapChain.h"
#include "platform/windows/graphics/vk/vkTextureManager.h"

namespace SirEngine::vk {
VkInstance INSTANCE = nullptr;
VkSurfaceKHR SURFACE = nullptr;
VkDevice LOGICAL_DEVICE = nullptr;
VkQueue GRAPHICS_QUEUE = nullptr;
VkQueue COMPUTE_QUEUE = nullptr;
VkQueue PRESENTATION_QUEUE = nullptr;
VkPhysicalDevice PHYSICAL_DEVICE = nullptr;
VkSwapchain *SWAP_CHAIN = nullptr;

VkFormat IMAGE_FORMAT = VK_FORMAT_UNDEFINED;
VkDebugReportCallbackEXT DEBUG_CALLBACK = nullptr;
VkDebugUtilsMessengerEXT DEBUG_CALLBACK2 = nullptr;

VkPSOManager *PSO_MANAGER = nullptr;
VkShaderManager *SHADER_MANAGER = nullptr;
VkConstantBufferManager *CONSTANT_BUFFER_MANAGER = nullptr;
VkPipelineLayoutManager *PIPELINE_LAYOUT_MANAGER = nullptr;
VkBufferManager *BUFFER_MANAGER = nullptr;
VkMeshManager *MESH_MANAGER = nullptr;
VkTextureManager *TEXTURE_MANAGER = nullptr;
VkMaterialManager *MATERIAL_MANAGER = nullptr;
VkBindingTableManager *DESCRIPTOR_MANAGER = nullptr;
VkDebugRenderer *DEBUG_RENDERER = nullptr;
uint32_t SWAP_CHAIN_IMAGE_COUNT = 0;
VkFrameCommand FRAME_COMMAND[PREALLOCATED_SEMAPHORE_COUNT];
VkFrameCommand *CURRENT_FRAME_COMMAND = nullptr;
uint32_t GRAPHICS_QUEUE_FAMILY = 0;
uint32_t PRESENTATION_QUEUE_FAMILY = 0;
bool DEBUG_MARKERS_ENABLED = false;

struct VkRenderable {
  VkMeshRuntime m_meshRuntime{};
  VkMaterialRuntime m_materialRuntime{};
};

typedef std::unordered_map<uint32_t, std::vector<VkRenderable>>
    VkRenderingQueues;

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
  AdapterRequestConfig adapterConfig;
  adapterConfig.m_vendor = globals::ENGINE_CONFIG->m_requestedAdapterVendor;
  adapterConfig.m_vendorTolerant = globals::ENGINE_CONFIG->m_vendorTolerant;
  adapterConfig.m_genericRule = globals::ENGINE_CONFIG->m_adapterSelectionRule;

  VkAdapterResult adapterResult{};
  const bool adapterFound = getBestAdapter(adapterConfig, adapterResult);
  assert(adapterFound);
  PHYSICAL_DEVICE = adapterResult.m_physicalDevice;
  LOGICAL_DEVICE = adapterResult.m_device;
  globals::ENGINE_CONFIG->m_selectdedAdapterVendor =
      adapterResult.m_foundVendor;
  if (globals::ENGINE_CONFIG->m_verboseStartup) {
    logPhysicalDevice(PHYSICAL_DEVICE);
  }

  // assert ballots extensions are available

  VkPhysicalDeviceSubgroupProperties subgroupProperties;
  subgroupProperties.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
  subgroupProperties.pNext = NULL;

  VkPhysicalDeviceProperties2 physicalDeviceProperties;
  physicalDeviceProperties.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  physicalDeviceProperties.pNext = &subgroupProperties;

  vkGetPhysicalDeviceProperties2(PHYSICAL_DEVICE, &physicalDeviceProperties);
  assert(((subgroupProperties.supportedOperations &
           VK_SUBGROUP_FEATURE_VOTE_BIT) > 0) &&
         "gpu does not support wave vote instructions");
  assert(((subgroupProperties.supportedOperations &
           VK_SUBGROUP_FEATURE_BALLOT_BIT) > 0) &&
         "gpu does not support wave ballot instructions");

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
  assert(SWAP_CHAIN_IMAGE_COUNT <= 9);  // used to convert easily the swap chain
                                        // image count to char for debug name

  // allocating all the per frame resources used for the render,
  // synchronization, command pool etc
  for (uint32_t i = 0; i < SWAP_CHAIN_IMAGE_COUNT; ++i) {
    char frame[2]{static_cast<char>(48 + globals::CURRENT_FRAME), '\0'};
    if (!newSemaphore(LOGICAL_DEVICE, (FRAME_COMMAND[i].m_acquireSemaphore))) {
      assert(0 && "failed to create acquire image semaphore");
    }
    SET_DEBUG_NAME(FRAME_COMMAND[i].m_acquireSemaphore,
                   VK_OBJECT_TYPE_SEMAPHORE,
                   frameConcatenation("acquireSemaphore", frame));

    if (!newSemaphore(LOGICAL_DEVICE, FRAME_COMMAND[i].m_renderSemaphore)) {
      assert(0 && "failed to create render semaphore");
    }
    SET_DEBUG_NAME(FRAME_COMMAND[i].m_renderSemaphore, VK_OBJECT_TYPE_SEMAPHORE,
                   frameConcatenation("renderSemaphore", frame));

    // Command buffers creation
    if (!createCommandPool(LOGICAL_DEVICE,
                           VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                           adapterResult.m_graphicsQueueFamilyIndex,
                           FRAME_COMMAND[i].m_commandAllocator)) {
      assert(0 && "could not create command pool");
    }
    SET_DEBUG_NAME(FRAME_COMMAND[i].m_commandAllocator,
                   VK_OBJECT_TYPE_COMMAND_POOL,
                   frameConcatenation("commandPool", frame));

    if (!allocateCommandBuffer(LOGICAL_DEVICE,
                               FRAME_COMMAND[i].m_commandAllocator,
                               VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                               FRAME_COMMAND[i].m_commandBuffer)) {
      assert(0);
    }
    SET_DEBUG_NAME(FRAME_COMMAND[i].m_commandBuffer,
                   VK_OBJECT_TYPE_COMMAND_BUFFER,
                   frameConcatenation("commandBuffer", frame));
  }

  CURRENT_FRAME_COMMAND = &FRAME_COMMAND[0];

  DESCRIPTOR_MANAGER = new VkBindingTableManager(10000, 10000);
  DESCRIPTOR_MANAGER->initialize();
  globals::BINDING_TABLE_MANAGER = DESCRIPTOR_MANAGER;

  SHADER_MANAGER = new VkShaderManager();
  SHADER_MANAGER->initialize();
  SHADER_MANAGER->loadShadersInFolder(
      frameConcatenation(globals::ENGINE_CONFIG->m_dataSourcePath,
                         "/processed/shaders/VK/rasterization"));
  SHADER_MANAGER->loadShadersInFolder(
      frameConcatenation(globals::ENGINE_CONFIG->m_dataSourcePath,
                         "/processed/shaders/VK/compute"));

  globals::SHADER_MANAGER = vk::SHADER_MANAGER;

  globals::LIGHT_MANAGER = new graphics::LightManager();

  PIPELINE_LAYOUT_MANAGER = new VkPipelineLayoutManager();
  PIPELINE_LAYOUT_MANAGER->initialize();

  globals::ROOT_SIGNATURE_MANAGER = PIPELINE_LAYOUT_MANAGER;

  PSO_MANAGER = new VkPSOManager();
  PSO_MANAGER->initialize();
  globals::PSO_MANAGER = PSO_MANAGER;
  // TODO TEMP HACK LOAD, remove this
  vk::PSO_MANAGER->loadRawPSO("../data/pso/HDRtoSDREffect_PSO.json");
  vk::PSO_MANAGER->loadRawPSO("../data/pso/forwardPhongPSO.json");
  vk::PSO_MANAGER->loadRawPSO("../data/pso/grassForwardPSO.json");
  vk::PSO_MANAGER->loadRawPSO("../data/pso/grassPlanePSO.json");
  vk::PSO_MANAGER->loadRawPSO("../data/pso/grassClearPSO.json");
  vk::PSO_MANAGER->loadRawPSO("../data/pso/grassCullingScanPSO.json");
  vk::PSO_MANAGER->loadRawPSO("../data/pso/debugDrawLinesSingleColorPSO.json");
  vk::PSO_MANAGER->loadRawPSO("../data/pso/skyboxPSO.json");
  vk::PSO_MANAGER->loadRawPSO("../data/pso/gammaAndToneMappingEffect_PSO.json");

  CONSTANT_BUFFER_MANAGER = new VkConstantBufferManager();
  CONSTANT_BUFFER_MANAGER->initialize();
  globals::CONSTANT_BUFFER_MANAGER = CONSTANT_BUFFER_MANAGER;

  BUFFER_MANAGER = new VkBufferManager();
  BUFFER_MANAGER->initialize();
  globals::BUFFER_MANAGER = BUFFER_MANAGER;

  MESH_MANAGER = new VkMeshManager();
  // MESH_MANAGER->initialize();
  globals::MESH_MANAGER = MESH_MANAGER;

  TEXTURE_MANAGER = new VkTextureManager();
  TEXTURE_MANAGER->initialize();
  globals::TEXTURE_MANAGER = TEXTURE_MANAGER;

  MATERIAL_MANAGER = new VkMaterialManager();
  MATERIAL_MANAGER->inititialize();
  MATERIAL_MANAGER->loadTypesInFolder(frameConcatenation(
      globals::ENGINE_CONFIG->m_dataSourcePath, "/materials/types"));
  globals::MATERIAL_MANAGER = MATERIAL_MANAGER;

  globals::DEBUG_RENDERER = new DebugRenderer();
  globals::DEBUG_RENDERER->initialize();

  globals::ASSET_MANAGER = new AssetManager();
  globals::ASSET_MANAGER->initialize();

  globals::INTEROP_DATA = new InteropData();
  globals::INTEROP_DATA->initialize();

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

RenderingContext *createVkRenderingContext(
    const RenderingContextCreationSettings &settings, uint32_t width,
    uint32_t height) {
  return new VkRenderingContext(settings, width, height);
}

VkRenderingContext::VkRenderingContext(
    const RenderingContextCreationSettings &settings, const uint32_t width,
    const uint32_t height)
    : RenderingContext(settings, width, height), m_bindingsPool(RESERVE_SIZE) {
  SE_CORE_INFO("Initializing a Vulkan context");
  queues = new VkRenderingQueues();
}

void setDebugNameImpl() {}
bool VkRenderingContext::initializeGraphics() {
  const bool result = vkInitializeGraphics(
      m_settings.window, m_screenInfo.width, m_screenInfo.height);
  if (!result) {
    SE_CORE_ERROR("FATAL: could not initialize graphics");
  }

  m_cameraHandle = globals::CONSTANT_BUFFER_MANAGER->allocate(
      sizeof(FrameData),
      ConstantBufferManager::CONSTANT_BUFFER_FLAG_BITS::UPDATED_EVERY_FRAME,
      nullptr);
  return result;
}

void VkRenderingContext::setupCameraForFrame() {
  globals::ACTIVE_CAMERA->updateCamera();

  m_frameData.m_mainCamera = globals::MAIN_CAMERA->getCameraBuffer();
  m_frameData.m_activeCamera = globals::ACTIVE_CAMERA->getCameraBuffer();

  m_frameData.screenWidth =
      static_cast<float>(globals::ENGINE_CONFIG->m_windowWidth);
  m_frameData.screenHeight =
      static_cast<float>(globals::ENGINE_CONFIG->m_windowHeight);
  m_frameData.time =
      static_cast<float>(globals::GAME_CLOCK.getDeltaFromOrigin() * 1e-9);

  globals::CONSTANT_BUFFER_MANAGER->update(m_cameraHandle, &m_frameData);

  // memcpy(m_cameraBuffer.data, &m_camBufferCPU, sizeof(m_camBufferCPU));
  globals::CONSTANT_BUFFER_MANAGER->update(m_cameraHandle, &m_frameData);

  VkWriteDescriptorSet writeDescriptorSets = {};
  VkDescriptorBufferInfo bufferInfoUniform = {};
  vk::CONSTANT_BUFFER_MANAGER->bindConstantBuffer(
      m_cameraHandle, bufferInfoUniform, 0, &writeDescriptorSets,
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(PER_FRAME_DATA_HANDLE));

  // camera update
  vkUpdateDescriptorSets(vk::LOGICAL_DEVICE, 1, &writeDescriptorSets, 0,
                         nullptr);
}

void VkRenderingContext::bindCameraBuffer(const RSHandle rs,
                                          bool isCompute) const {
  VkDescriptorSet descriptorSet =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(PER_FRAME_DATA_HANDLE);

  VkPipelineLayout layout =
      vk::PIPELINE_LAYOUT_MANAGER->getLayoutFromHandle(rs);
  assert(layout != nullptr);

  VkDescriptorSet sets[] = {
      descriptorSet,
  };
  auto bindPoint = isCompute ? VK_PIPELINE_BIND_POINT_COMPUTE
                             : VK_PIPELINE_BIND_POINT_GRAPHICS;
  vkCmdBindDescriptorSets(CURRENT_FRAME_COMMAND->m_commandBuffer, bindPoint,
                          layout, PSOManager::PER_FRAME_DATA_BINDING_INDEX, 1,
                          sets, 0, nullptr);
}

void waitOnFence(VkFence fence) {
  if (fence == nullptr) {
    return;
  }
  VK_CHECK(vkWaitForFences(vk::LOGICAL_DEVICE, 1, &fence, true, 200000000));
}

void resetFrameCommand(VkFrameCommand *command) {
  vkResetCommandPool(LOGICAL_DEVICE, command->m_commandAllocator, 0);
  if (command->m_endOfFrameFence == nullptr) {
    VkFenceCreateInfo create_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VK_CHECK(vkCreateFence(LOGICAL_DEVICE, &create_info, nullptr,
                           &command->m_endOfFrameFence));
    assert(globals::CURRENT_FRAME <= 9);
    // bit brute force but at least i dont need std::to_string
    char frame[2]{static_cast<char>(48 + globals::CURRENT_FRAME), '\0'};
    SET_DEBUG_NAME(command->m_endOfFrameFence, VK_OBJECT_TYPE_FENCE,
                   frameConcatenation("endOfFrameFence", frame));
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
      VK_QUEUE_FAMILY_IGNORED,  // used for cross queue sync
      VK_QUEUE_FAMILY_IGNORED,
      VK_IMAGE_ASPECT_COLOR_BIT};  // this wont work if you have depth buffers

  setImageMemoryBarrier(CURRENT_FRAME_COMMAND->m_commandBuffer,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        {imageTransitionBeforeDrawing});

  // let us bind static samplers and camera
  VkDescriptorSet sets[] = {
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(PER_FRAME_DATA_HANDLE),
      vk::STATIC_SAMPLERS_DESCRIPTOR_SET};

  // multiple descriptor sets
  vkCmdBindDescriptorSets(CURRENT_FRAME_COMMAND->m_commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          vk::ENGINE_PIPELINE_LAYOUT, 0, 2, sets, 0, nullptr);

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

  SHADER_MANAGER->cleanup();
  globals::DEBUG_RENDERER->cleanup();
  globals::LIGHT_MANAGER->cleanup();
  CONSTANT_BUFFER_MANAGER->cleanup();

  for (uint32_t i = 0; i < SWAP_CHAIN_IMAGE_COUNT; ++i) {
    vkDestroySemaphore(LOGICAL_DEVICE, FRAME_COMMAND[i].m_acquireSemaphore,
                       nullptr);
    vkDestroySemaphore(LOGICAL_DEVICE, FRAME_COMMAND[i].m_renderSemaphore,
                       nullptr);
    vkDestroyCommandPool(LOGICAL_DEVICE, FRAME_COMMAND[i].m_commandAllocator,
                         nullptr);
    vkDestroyFence(LOGICAL_DEVICE, FRAME_COMMAND[i].m_endOfFrameFence, nullptr);
  }
  // destroying environment texture handles
  if (m_enviromentMapHandle.isHandleValid()) {
    vk::TEXTURE_MANAGER->free(m_enviromentMapHandle);
  }
  if (m_enviromentMapIrradianceHandle.isHandleValid()) {
    vk::TEXTURE_MANAGER->free(m_enviromentMapIrradianceHandle);
  }
  if (m_enviromentMapRadianceHandle.isHandleValid()) {
    vk::TEXTURE_MANAGER->free(m_enviromentMapRadianceHandle);
  }
  if (m_brdfHandle.isHandleValid()) {
    vk::TEXTURE_MANAGER->free(m_brdfHandle);
  }

  // clean up manager
  PIPELINE_LAYOUT_MANAGER->cleanup();
  PSO_MANAGER->cleanup();

  MATERIAL_MANAGER->releaseAllMaterialsAndRelatedResources();

  TEXTURE_MANAGER->cleanup();
  DESCRIPTOR_MANAGER->cleanup();

  vkDestroyDevice(LOGICAL_DEVICE, nullptr);
  vkDestroySurfaceKHR(INSTANCE, SURFACE, nullptr);
  // vkDestroyDebugReportCallbackEXT(INSTANCE, DEBUG_CALLBACK, nullptr);
  vkDestroyDebugUtilsMessengerEXT(INSTANCE, DEBUG_CALLBACK2, nullptr);
  vkDestroyInstance(INSTANCE, nullptr);
  return true;
}

void VkRenderingContext::flush() { vkDeviceWaitIdle(LOGICAL_DEVICE); }

void VkRenderingContext::executeGlobalCommandList() {
  auto buffer = CURRENT_FRAME_COMMAND->m_commandBuffer;

  if (buffer == VK_NULL_HANDLE) {
    return;
  }

  VK_CHECK(vkEndCommandBuffer(buffer));

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &buffer;

  // Submit to the queue
  VK_CHECK(vkQueueSubmit(GRAPHICS_QUEUE, 1, &submitInfo, nullptr));
}

void VkRenderingContext::resetGlobalCommandList() {
  vkResetCommandPool(LOGICAL_DEVICE, CURRENT_FRAME_COMMAND->m_commandAllocator,
                     0);
  beginCommandBufferRecordingOperation(
      CURRENT_FRAME_COMMAND->m_commandBuffer,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
}

void VkRenderingContext::addRenderablesToQueue(const Renderable &renderable) {
  auto *typedQueues = reinterpret_cast<VkRenderingQueues *>(queues);

  VkRenderable vkRenderable{};

  const VkMaterialRuntime &materialRuntime =
      vk::MATERIAL_MANAGER->getMaterialRuntime(renderable.m_materialHandle);
  const VkMeshRuntime &meshRuntime =
      vk::MESH_MANAGER->getMeshRuntime(renderable.m_meshHandle);

  vkRenderable.m_materialRuntime = materialRuntime;
  vkRenderable.m_meshRuntime = meshRuntime;
  // store the renderable on each queue
  for (int i = 0; i < MaterialManager::QUEUE_COUNT; ++i) {
    const uint32_t flag = materialRuntime.shaderQueueTypeFlags[i];

    if (flag != INVALID_QUEUE_TYPE_FLAGS) {
      (*typedQueues)[flag].emplace_back(vkRenderable);
    }
  }
}

void VkRenderingContext::renderQueueType(
    const DrawCallConfig &config, const SHADER_QUEUE_FLAGS flag,
    const BindingTableHandle passBindings) {
  const auto &typedQueues = *(reinterpret_cast<VkRenderingQueues *>(queues));

  auto *currentFc = CURRENT_FRAME_COMMAND;
  auto commandList = currentFc->m_commandBuffer;

  // draw calls go here
  VkViewport viewport{0,
                      float(config.height),
                      float(config.width),
                      -float(config.height),
                      0.0f,
                      1.0f};
  VkRect2D scissor{{0, 0},
                   {static_cast<uint32_t>(config.width),
                    static_cast<uint32_t>(config.height)}};
  vkCmdSetViewport(commandList, 0, 1, &viewport);
  vkCmdSetScissor(commandList, 0, 1, &scissor);

  for (const auto &renderableList : typedQueues) {
    if (MATERIAL_MANAGER->isQueueType(renderableList.first, flag)) {
      // now that we know the material goes in the the deferred queue we can
      // start rendering it

      // bind the corresponding RS and PSO
      ShaderBind bind =
          vk::MATERIAL_MANAGER->bindRSandPSO(renderableList.first, commandList);

      // this is most for debug, it will boil down to nothing in release
      const SHADER_TYPE_FLAGS type =
          MATERIAL_MANAGER->getTypeFlags(renderableList.first);
      const std::string &typeName =
          MATERIAL_MANAGER->getStringFromShaderTypeFlag(type);

      bindCameraBuffer(bind.rs);

      if (passBindings.isHandleValid()) {
        globals::BINDING_TABLE_MANAGER->bindTable(
            PSOManager::PER_PASS_BINDING_INDEX, passBindings, bind.rs);
      }
      annotateGraphicsBegin(typeName.c_str());

      // looping each of the object
      const size_t count = renderableList.second.size();
      const VkRenderable *currRenderables = renderableList.second.data();
      for (size_t i = 0; i < count; ++i) {
        const VkRenderable &renderable = currRenderables[i];

        // bind material data like textures etc, then render
        MATERIAL_MANAGER->bindMaterial(flag, renderable.m_materialRuntime,
                                       commandList);

        MESH_MANAGER->renderMesh(renderable.m_meshRuntime, commandList);
      }
      annotateGraphicsEnd();
    }
  }
}

VkRenderPass createRenderPass(const FrameBufferBindings &bindings,
                              const char *name) {
  VkAttachmentDescription attachments[10] = {};
  VkAttachmentReference attachmentsRefs[10] = {};
  int count = 0;
  for (int i = 0; i < 8; ++i) {
    const RTBinding &binding = bindings.colorRT[i];
    if ((!binding.handle.isHandleValid()) & (!binding.isSwapChainBackBuffer)) {
      continue;
    }

    VkFormat currentFormat =
        binding.isSwapChainBackBuffer
            ? vk::IMAGE_FORMAT
            : vk::TEXTURE_MANAGER->getTextureFormat(binding.handle);
    assert(currentFormat != VK_FORMAT_UNDEFINED && "Unsupported render format");

    attachments[count].format = currentFormat;
    // TODO no MSAA yet
    attachments[count].samples = VK_SAMPLE_COUNT_1_BIT;
    // attachments[count].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[count].loadOp = binding.shouldClearColor
                                    ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                    : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // TODO not really sure what to do about the stencil...
    // for now set to load and store, should leave it untouched
    // attachments[count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    // attachments[count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[count].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[count].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // we have only one subpass and uses all attachments
    attachmentsRefs[count].attachment = count;
    attachmentsRefs[count].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    count++;
  }

  bool hasDepth = bindings.depthStencil.handle.isHandleValid();
  if (hasDepth) {
    const DepthBinding &binding = bindings.depthStencil;
    VkFormat currentFormat =
        vk::TEXTURE_MANAGER->getTextureFormat(binding.handle);
    assert(currentFormat != VK_FORMAT_UNDEFINED && "Unsupported render format");

    attachments[count].format = currentFormat;
    // TODO no MSAA yet
    attachments[count].samples = VK_SAMPLE_COUNT_1_BIT;
    // attachments[count].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[count].loadOp = binding.shouldClearDepth
                                    ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                    : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // TODO not really sure what to do about the stencil...
    // for now set to load and store, should leave it untouched
    attachments[count].stencilLoadOp = binding.shouldClearStencil
                                           ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                           : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ;
    attachments[count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[count].initialLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[count].finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // we have only one subpass and uses all attachments
    attachmentsRefs[count].attachment = count;
    attachmentsRefs[count].layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    count++;
  }

  VkRenderPass renderPass{};

  VkSubpassDescription subPass{};
  subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subPass.colorAttachmentCount = hasDepth ? count - 1 : count;
  subPass.pColorAttachments = attachmentsRefs;
  subPass.pDepthStencilAttachment =
      hasDepth ? &attachmentsRefs[count - 1] : nullptr;

  VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  createInfo.attachmentCount = count;
  createInfo.pAttachments = attachments;
  createInfo.subpassCount = 1;
  createInfo.pSubpasses = &subPass;

  vkCreateRenderPass(vk::LOGICAL_DEVICE, &createInfo, nullptr, &renderPass);
  SET_DEBUG_NAME(renderPass, VK_OBJECT_TYPE_RENDER_PASS,
                 frameConcatenation(name, "RenderPass"));
  return renderPass;
}
VkFramebuffer *createFrameBuffer(VkRenderPass pass,
                                 const FrameBufferBindings &bindings,
                                 const char *name,
                                 uint32_t &outFrameBufferCount) {
  bool bindsToBackBuffer = false;
  for (int i = 0; i < 8; ++i) {
    bindsToBackBuffer |= bindings.colorRT[i].isSwapChainBackBuffer;
  }

  // if binds to backBuffer we need to create as many frame buffers
  // as there are swap chain images
  outFrameBufferCount = bindsToBackBuffer ? vk::SWAP_CHAIN_IMAGE_COUNT : 1u;
  VkFramebuffer *frameBuffers =
      reinterpret_cast<VkFramebuffer *>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(VkFramebuffer) * outFrameBufferCount));

  VkImageView imageViews[10]{};
  for (uint32_t swap = 0; swap < outFrameBufferCount; ++swap) {
    int count = 0;
    for (int i = 0; i < 8; ++i) {
      const RTBinding &binding = bindings.colorRT[i];
      if (!binding.handle.isHandleValid() & (!binding.isSwapChainBackBuffer)) {
        continue;
      }
      VkImageView view =
          binding.isSwapChainBackBuffer
              ? vk::SWAP_CHAIN->imagesView[swap]
              : vk::TEXTURE_MANAGER->getTextureData(bindings.colorRT[i].handle)
                    .view;
      imageViews[i] = view;
      ++count;
    }

    bool hasDepth = bindings.depthStencil.handle.isHandleValid();
    if (hasDepth) {
      const DepthBinding &binding = bindings.depthStencil;
      VkImageView view =
          vk::TEXTURE_MANAGER->getTextureData(binding.handle).view;
      imageViews[count] = view;
      count++;
    }

    VkFramebufferCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    createInfo.renderPass = pass;
    createInfo.pAttachments = imageViews;
    createInfo.attachmentCount = count;
    createInfo.width = bindings.width;
    createInfo.height = bindings.height;
    createInfo.layers = 1;
    VkFramebuffer buffer;

    VK_CHECK(vkCreateFramebuffer(vk::LOGICAL_DEVICE, &createInfo, nullptr,
                                 &(frameBuffers[swap])));
  }
  return frameBuffers;
}

BufferBindingsHandle VkRenderingContext::prepareBindingObject(
    const FrameBufferBindings &bindings, const char *name) {
  uint32_t index;
  FrameBindingsData &data = m_bindingsPool.getFreeMemoryData(index);
  data.m_pass = createRenderPass(bindings, name);
  data.m_buffer =
      createFrameBuffer(data.m_pass, bindings, name, data.m_frameBufferCount);
  data.name = persistentString(name);
  data.m_bindings = bindings;
  data.m_magicNumber = MAGIC_NUMBER_COUNTER++;
  return {data.m_magicNumber << 16 | index};
}

void VkRenderingContext::clearBindingObject(const BufferBindingsHandle) {
  vkCmdEndRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
}

void VkRenderingContext::freeBindingObject(const BufferBindingsHandle handle) {
  assertMagicNumber(handle);
  const uint32_t magic = getMagicFromHandle(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  const FrameBindingsData &data = m_bindingsPool.getConstRef(idx);
  vkDestroyRenderPass(vk::LOGICAL_DEVICE, data.m_pass, nullptr);
  for (uint32_t i = 0; i < data.m_frameBufferCount; ++i) {
    vkDestroyFramebuffer(vk::LOGICAL_DEVICE, data.m_buffer[i], nullptr);
  }
  globals::PERSISTENT_ALLOCATOR->free(data.m_buffer);
  stringFree(data.name);

  m_bindingsPool.free(idx);
}

void VkRenderingContext::fullScreenPass() {
  VkCommandBuffer buffer = vk::CURRENT_FRAME_COMMAND->m_commandBuffer;
  vkCmdDraw(buffer, 6, 1, 0, 0);
}

void VkRenderingContext::setViewportAndScissor(
    const float offsetX, const float offsetY, const float width,
    const float height, const float minDepth, const float maxDepth) {
  auto *currentFc = CURRENT_FRAME_COMMAND;
  auto commandList = currentFc->m_commandBuffer;

  // draw calls go here
  // here the standard position of the viewport is the same as height due to
  // inverted viewport so we get height -offsetY to get the correct value
  VkViewport viewport{offsetX, height - offsetY, width,
                      -height, minDepth,         maxDepth};
  VkRect2D scissor{
      {static_cast<int32_t>(offsetX), static_cast<int32_t>(offsetY)},
      {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}};
  vkCmdSetViewport(commandList, 0, 1, &viewport);
  vkCmdSetScissor(commandList, 0, 1, &scissor);
}

void VkRenderingContext::renderProcedural(const uint32_t indexCount) {
  auto *currentFc = CURRENT_FRAME_COMMAND;
  VkCommandBuffer commandList = currentFc->m_commandBuffer;
  vkCmdDraw(commandList, indexCount, 1, 0, 0);
}

void VkRenderingContext::dispatchCompute(const uint32_t blockX,
                                         const uint32_t blockY,
                                         const uint32_t blockZ) {
  auto *currentFc = CURRENT_FRAME_COMMAND;
  VkCommandBuffer commandList = currentFc->m_commandBuffer;
  vkCmdDispatch(commandList, blockX, blockY, blockZ);
}

void VkRenderingContext::renderProceduralIndirect(
    const BufferHandle &argsBuffer, const RSHandle handle) {
  auto *currentFc = CURRENT_FRAME_COMMAND;
  VkCommandBuffer commandList = currentFc->m_commandBuffer;
  auto bufferData = vk::BUFFER_MANAGER->getBufferData(argsBuffer);
  vkCmdDrawIndirect(commandList, bufferData.buffer, 0, 1, 0);
}

int vkBarrier(int counter, VkImageMemoryBarrier *barriers,
              const TextureHandle handle, const RESOURCE_STATE oldState,
              const RESOURCE_STATE newState) {
  if (oldState == newState) {
    return counter;
  }
  VkTexture2D rt = vk::TEXTURE_MANAGER->getTextureData(handle);
  VkImageLayout oldLayout = fromStateToLayout(oldState);
  VkImageLayout newLayout = fromStateToLayout(newState);
  barriers[counter].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barriers[counter].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  barriers[counter].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barriers[counter].srcQueueFamilyIndex = 0;
  barriers[counter].dstQueueFamilyIndex = 0;
  barriers[counter].image = rt.image;

  barriers[counter].oldLayout = oldLayout;
  barriers[counter].newLayout = newLayout;
  barriers[counter].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barriers[counter].subresourceRange.baseArrayLayer = 0;
  barriers[counter].subresourceRange.baseMipLevel = 0;
  barriers[counter].subresourceRange.levelCount = 1;
  barriers[counter].subresourceRange.layerCount = 1;
  return ++counter;
}

void VkRenderingContext::setBindingObject(const BufferBindingsHandle handle) {
  assertMagicNumber(handle);
  const uint32_t idx = getIndexFromHandle(handle);
  const FrameBindingsData &data = m_bindingsPool.getConstRef(idx);

  VkImageMemoryBarrier barriers[10]{};
  int barrierCounter = 0;

  VkClearValue colors[10]{};
  int count = 0;
  for (int i = 0; i < 8; ++i) {
    const RTBinding &binding = data.m_bindings.colorRT[i];
    if (!binding.handle.isHandleValid()) {
      continue;
    }
    count += binding.shouldClearColor ? 1 : 0;
    colors[i].color = {binding.clearColor.r, binding.clearColor.g,
                       binding.clearColor.b, binding.clearColor.a};
    barrierCounter =
        vkBarrier(barrierCounter, barriers, binding.handle,
                  binding.currentResourceState, binding.neededResourceState);
  }

  for (uint32_t i = 0; i < data.m_bindings.extraBindingsCount; ++i) {
    const RTBinding &binding = data.m_bindings.extraBindings[i];
    if (!binding.handle.isHandleValid()) {
      continue;
    }
    barrierCounter =
        vkBarrier(barrierCounter, barriers, binding.handle,
                  binding.currentResourceState, binding.neededResourceState);
  }

  // taking care of depth
  bool hasDepth = data.m_bindings.depthStencil.handle.isHandleValid();
  if (hasDepth) {
    const DepthBinding &binding = data.m_bindings.depthStencil;
    colors[count].depthStencil = {
        binding.clearDepthColor.r,
        static_cast<uint32_t>(binding.clearStencilColor.g)};
    count += binding.shouldClearDepth ? 1 : 0;
    barrierCounter =
        vkBarrier(barrierCounter, barriers, binding.handle,
                  binding.currentResourceState, binding.neededResourceState);
  }

  if (barrierCounter) {
    vkCmdPipelineBarrier(vk::CURRENT_FRAME_COMMAND->m_commandBuffer,
                         VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                         VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                         VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 0, nullptr,
                         barrierCounter, barriers);
  }

  VkRenderPassBeginInfo beginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                     nullptr};
  beginInfo.renderPass = data.m_pass;
  uint32_t bufferIdx =
      data.m_frameBufferCount == 1 ? 0 : globals::CURRENT_FRAME;
  beginInfo.framebuffer = data.m_buffer[bufferIdx];

  // similar to a viewport mostly used on "tiled renderers" to optimize, talking
  // about hardware based tile renderer, aka mobile GPUs.
  beginInfo.renderArea.extent.width =
      static_cast<int32_t>(globals::ENGINE_CONFIG->m_windowWidth);
  beginInfo.renderArea.extent.height =
      static_cast<int32_t>(globals::ENGINE_CONFIG->m_windowHeight);
  beginInfo.clearValueCount = count;
  beginInfo.pClearValues = colors;

  vkCmdBeginRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, &beginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
}
}  // namespace SirEngine::vk
