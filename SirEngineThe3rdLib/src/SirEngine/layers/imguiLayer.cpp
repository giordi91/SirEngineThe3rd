//#include <Windows.h>

#include "imguiLayer.h"

#include "SirEngine/core.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "imgui/imgui.h"

#if BUILD_DX12
// DX12
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/dx12SwapChain.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"
#include "platform/windows/graphics/dx12/imgui_impl_dx12.h"
#endif

#if BUILD_VK
// VK
#include "platform/windows/graphics/vk/imgui_impl_vulkan.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkBindingTableManager.h"
#include "platform/windows/graphics/vk/vkSwapChain.h"
#endif

#include "SirEngine/application.h"
#include "SirEngine/input.h"
#include "SirEngine/events/applicationEvent.h"
#include "SirEngine/events/event.h"
#include "SirEngine/events/keyboardEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/events/renderGraphEvent.h"
#include "SirEngine/events/scriptingEvent.h"
#include "SirEngine/events/shaderCompileEvent.h"
#include "SirEngine/graphics/debugAnnotations.h"

namespace SirEngine {
void ImguiLayer::onAttach() {
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
    // need to initialize ImGui dx12
    dx12::DescriptorPair pair;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->reserveDescriptor(pair);
    ImGui_ImplDX12_Init(dx12::DEVICE, FRAME_BUFFERS_COUNT,
                        DXGI_FORMAT_R8G8B8A8_UNORM, pair.cpuHandle,
                        pair.gpuHandle);
  } else {
    assert(globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::VULKAN);

#if BUILD_VK
    ImGui_ImplVulkan_InitInfo vkinfo{};
    vkinfo.Instance = vk::INSTANCE;
    vkinfo.PhysicalDevice = vk::PHYSICAL_DEVICE;
    vkinfo.Device = vk::LOGICAL_DEVICE;
    vkinfo.QueueFamily = vk::GRAPHICS_QUEUE_FAMILY;
    vkinfo.Queue = vk::GRAPHICS_QUEUE;
    vkinfo.PipelineCache = nullptr;
    vkinfo.DescriptorPool = vk::DESCRIPTOR_MANAGER->getPool();
    vkinfo.Allocator = nullptr;
    vkinfo.ImageCount = vk::SWAP_CHAIN_IMAGE_COUNT;
    vkinfo.MinImageCount = vk::SWAP_CHAIN_IMAGE_COUNT;

    VkAttachmentDescription attachment = {};
    attachment.format = vk::IMAGE_FORMAT;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;
    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    VK_CHECK(
        vkCreateRenderPass(vk::LOGICAL_DEVICE, &info, nullptr, &imguiPass));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplVulkan_Init(&vkinfo, imguiPass);

    // create a command buffer separated to execute this stuff

    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(vk::CURRENT_FRAME_COMMAND->m_commandBuffer,
                         &beginInfo);
    ImGui_ImplVulkan_CreateFontsTexture(
        vk::CURRENT_FRAME_COMMAND->m_commandBuffer);

    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &vk::CURRENT_FRAME_COMMAND->m_commandBuffer;
    VK_CHECK(vkEndCommandBuffer(vk::CURRENT_FRAME_COMMAND->m_commandBuffer));
    VK_CHECK(vkQueueSubmit(vk::GRAPHICS_QUEUE, 1, &end_info, VK_NULL_HANDLE));
    VK_CHECK(vkDeviceWaitIdle(vk::LOGICAL_DEVICE));
    vkResetCommandBuffer(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, 0);
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    ImGuiStyle &style = ImGui::GetStyle();
    if (globals::ENGINE_CONFIG->m_selectdedAdapterVendor ==
        ADAPTER_VENDOR::AMD) {
      style.Colors[ImGuiCol_WindowBg] = ImVec4(0.4f, 0.0f, 0.0f, 0.75f);
      style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.7f, 0.0f, 0.0f, 1.00f);
    } else if (globals::ENGINE_CONFIG->m_selectdedAdapterVendor ==
               ADAPTER_VENDOR::NVIDIA) {
      style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.4f, 0.0f, 0.75f);
      style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.7f, 0.0f, 1.00f);
    }
#endif
  }

  // Keyboard mapping. ImGui will use those indices to peek into the
  // io.KeysDown[] array that we will update during the application lifetime.
  ImGuiIO &io = ImGui::GetIO();
  io.KeyMap[ImGuiKey_Tab] = VK_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
  io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
  io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
  io.KeyMap[ImGuiKey_Home] = VK_HOME;
  io.KeyMap[ImGuiKey_End] = VK_END;
  io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
  io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
  io.KeyMap[ImGuiKey_Space] = VK_SPACE;
  io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
  io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
  io.KeyMap[ImGuiKey_A] = 'A';
  io.KeyMap[ImGuiKey_C] = 'C';
  io.KeyMap[ImGuiKey_V] = 'V';
  io.KeyMap[ImGuiKey_X] = 'X';
  io.KeyMap[ImGuiKey_Y] = 'Y';
  io.KeyMap[ImGuiKey_Z] = 'Z';

  ::QueryPerformanceFrequency(
      reinterpret_cast<LARGE_INTEGER *>(&g_TicksPerSecond));
  ::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&g_Time));

  io.DisplaySize =
      ImVec2(static_cast<float>(globals::ENGINE_CONFIG->m_windowWidth),
             static_cast<float>(globals::ENGINE_CONFIG->m_windowHeight));

  m_renderGraph.initialize(globals::RENDERING_GRAPH);
  m_shaderWidget.initialize();
  m_grassWidget.initialize();
}

void ImguiLayer::onDetach() { ImGui_ImplDX12_Shutdown(); }

void ImguiLayer::onUpdate() {
  if (!m_shouldShow) {
    return;
  }

  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
    annotateGraphicsBegin("UI Draw");
    TextureHandle destination = dx12::SWAP_CHAIN->currentBackBufferTexture();
    D3D12_RESOURCE_BARRIER barriers[1];
    int counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
        destination, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, 0);
    if (counter) {
      dx12::CURRENT_FRAME_RESOURCE->fc.commandList->ResourceBarrier(counter,
                                                                    barriers);
    }

    dx12::TEXTURE_MANAGER->bindBackBuffer();
    ImGui_ImplDX12_NewFrame();
  }

  // Read keyboard modifiers inputs
  ImGuiIO &io = ImGui::GetIO();

  // Setup time step
  INT64 current_time;
  ::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&current_time));
  io.DeltaTime = static_cast<float>(current_time - g_Time) / g_TicksPerSecond;
  g_Time = current_time;

  // Read keyboard modifiers inputs
  io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
  io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
  io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
  io.KeySuper = false;

  IM_ASSERT(io.Fonts->IsBuilt() &&
            "Font atlas not built! It is generally built by the renderer "
            "back-end. Missing call to renderer _NewFrame() function? e.g. "
            "ImGui_ImplOpenGL3_NewFrame().");

  bool showDemoWindow = true;

  const ImVec2 pos{0, 0};
  ImGui::NewFrame();
  // ImGui::ShowDemoWindow(&show_demo_window);

  ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
  ImGui::Begin("Debug", &showDemoWindow, ImGuiWindowFlags_MenuBar);
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Tools")) {
      if (ImGui::MenuItem("Shader compiler", nullptr)) {
        m_renderShaderCompiler = !m_renderShaderCompiler;
      }
      if (ImGui::MenuItem("Grass Tool Config", nullptr)) {
        m_renderGrassConfig= !m_renderGrassConfig;
      }
      if (ImGui::MenuItem("Reload scripts", nullptr)) {
        SE_CORE_INFO("reload scripts");
        auto *event = new ReloadScriptsEvent();
        globals::APPLICATION->queueEventForEndOfFrame(event);
      }
      ImGui::Separator();
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

#if BUILD_AMD
  if (ImGui::CollapsingHeader("HW info", ImGuiTreeNodeFlags_DefaultOpen)) {
    m_hwInfo.render();
  }
#endif

  if (ImGui::CollapsingHeader("Performances", ImGuiTreeNodeFlags_DefaultOpen)) {
    m_frameTimings.render();
    m_memoryUsage.render();
  }
  if (ImGui::CollapsingHeader("Graphics", ImGuiTreeNodeFlags_DefaultOpen)) {
    m_renderGraph.render();
  }
  if (m_renderShaderCompiler) {
    m_shaderWidget.render();
  }
  if (m_renderGrassConfig) {
    m_grassWidget.render();
  }

  ImGui::End();
  ImGui::Render();

  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
#if BUILD_DX12
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
                                  dx12::CURRENT_FRAME_RESOURCE->fc.commandList);
#endif
    annotateGraphicsEnd();
  } else {
#if BUILD_VK
    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = imguiPass;
    info.framebuffer = vk::SWAP_CHAIN->frameBuffers[globals::CURRENT_FRAME];
    info.renderArea.extent.width = globals::ENGINE_CONFIG->m_windowWidth;
    info.renderArea.extent.height = globals::ENGINE_CONFIG->m_windowHeight;
    info.clearValueCount = 0;
    info.pClearValues = nullptr;
    vkCmdBeginRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, &info,
                         VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
    // Submit command buffer
    vkCmdEndRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
#endif
  }
}
/*
else {
  // Read keyboard modifiers inputs
  ImGuiIO &io = ImGui::GetIO();
  // Setup time step
  INT64 current_time;
  ::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&current_time));
  io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
  // SE_CORE_INFO("time {0}", io.DeltaTime);
  g_Time = current_time;

  // Read keyboard modifiers inputs
  io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
  io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
  io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
  io.KeySuper = false;

  ImGui_ImplVulkan_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("Performance Metrics"); // Create a window called "Hello,
                                       // world!" and append into it.

  ImGui::Text("GPU Time: %.1f ms", 999999);

  ImGui::Text("CPU Time: %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::Separator();
  ImGui::Text("Swapchain Images: %d", vk::SWAP_CHAIN_IMAGE_COUNT);

  ImGui::End();

  ImGui::Render();

  VkRenderPassBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  info.renderPass = imguiPass;
  info.framebuffer = vk::SWAP_CHAIN->frameBuffers[globals::CURRENT_FRAME];
  info.renderArea.extent.width = globals::ENGINE_CONFIG->m_windowWidth;
  info.renderArea.extent.height = globals::ENGINE_CONFIG->m_windowHeight;
  info.clearValueCount = 0;
  info.pClearValues = nullptr;
  vkCmdBeginRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer, &info,
                       VK_SUBPASS_CONTENTS_INLINE);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
  // Submit command buffer
  vkCmdEndRenderPass(vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
*/
//} // namespace SirEngine


#define SE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
void ImguiLayer::onEvent(Event &event) {
  EventDispatcher dispatcher(event);
  dispatcher.dispatch<KeyTypeEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::onKeyTypeEvent));
  dispatcher.dispatch<MouseButtonPressEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::onMouseButtonPressEvent));
  dispatcher.dispatch<MouseButtonReleaseEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::onMouseButtonReleaseEvent));
  dispatcher.dispatch<MouseMoveEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::onMouseMoveEvent));
  dispatcher.dispatch<MouseScrollEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::onMouseScrolledEvent));
  dispatcher.dispatch<KeyboardPressEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::onKeyPressedEvent));
  dispatcher.dispatch<KeyboardReleaseEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::onKeyReleasedEvent));
  dispatcher.dispatch<WindowResizeEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::onWindowResizeEvent));
  dispatcher.dispatch<RenderGraphChanged>(
      SE_BIND_EVENT_FN(ImguiLayer::onRenderGraphEvent));
  dispatcher.dispatch<ShaderCompileResultEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::onCompileResultEvent));
  dispatcher.dispatch<RequestShaderCompileEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::onRequestCompileEvent));
}
#undef SE_BIND_EVENT_FN

void ImguiLayer::clear() {
#if BUILD_VK
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::VULKAN) {
    VK_CHECK(vkDeviceWaitIdle(vk::LOGICAL_DEVICE));
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
    vkDestroyRenderPass(vk::LOGICAL_DEVICE, imguiPass, nullptr);
  }
#endif
}

bool ImguiLayer::onMouseButtonPressEvent(const MouseButtonPressEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.MouseDown[static_cast<int>(e.getMouseButton())] = true;
  globals::INPUT->m_uiCapturingMouse =io.WantCaptureMouse; 
  return io.WantCaptureMouse;
}
bool ImguiLayer::onMouseButtonReleaseEvent(
    const MouseButtonReleaseEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.MouseDown[static_cast<int>(e.getMouseButton())] = false;
  return false;
}
bool ImguiLayer::onMouseMoveEvent(const MouseMoveEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.MousePos = ImVec2(e.getX(), e.getY());
  globals::INPUT->m_uiCapturingMouse =io.WantCaptureMouse; 
  return io.WantCaptureMouse;
}
bool ImguiLayer::onMouseScrolledEvent(const MouseScrollEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.MouseWheelH += e.getOffsetX();
  io.MouseWheel += e.getOffsetY();
  globals::INPUT->m_uiCapturingMouse =io.WantCaptureMouse; 
  return io.WantCaptureMouse;
}
bool ImguiLayer::onKeyPressedEvent(const KeyboardPressEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  int c = e.getKeyCode();
  io.KeysDown[c] = true;
  if (c == TRIGGER_UI_BUTTON) {
    m_shouldShow = !m_shouldShow;
  }
  globals::INPUT->m_uiCapturingKeyboard =io.WantCaptureKeyboard; 
  return io.WantCaptureKeyboard;
}
bool ImguiLayer::onKeyReleasedEvent(const KeyboardReleaseEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.KeysDown[e.getKeyCode()] = false;
  globals::INPUT->m_uiCapturingKeyboard =io.WantCaptureKeyboard; 
  return io.WantCaptureKeyboard;
}
bool ImguiLayer::onWindowResizeEvent(const WindowResizeEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(static_cast<float>(e.getWidth()),
                          static_cast<float>(e.getHeight()));
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
  ImGui_ImplDX12_InvalidateDeviceObjects();

  return false;
}
bool ImguiLayer::onKeyTypeEvent(const KeyTypeEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.AddInputCharacter(static_cast<uint16_t>(e.getKeyCode()));
  globals::INPUT->m_uiCapturingKeyboard =io.WantCaptureKeyboard; 
  return io.WantCaptureKeyboard;
}

bool ImguiLayer::onRenderGraphEvent(const RenderGraphChanged &) {
  m_renderGraph.initialize(globals::RENDERING_GRAPH);
  m_renderGraph.showGraph(true);
  return true;
}
bool ImguiLayer::onCompileResultEvent(const ShaderCompileResultEvent &e) {
  m_shaderWidget.log(e.getLog());
  return true;
}

bool ImguiLayer::onRequestCompileEvent(const RequestShaderCompileEvent &) {
  m_shaderWidget.requestCompile();
  return true;
}
}  // namespace SirEngine
