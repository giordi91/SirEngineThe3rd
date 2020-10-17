
#include "SirEngine/Layers/editorLayer.h"

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

#include <imgui/imgui_internal.h>

#include "SirEngine/application.h"
#include "SirEngine/events/applicationEvent.h"
#include "SirEngine/events/event.h"
#include "SirEngine/events/keyboardEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/events/renderGraphEvent.h"
#include "SirEngine/events/scriptingEvent.h"
#include "SirEngine/events/shaderCompileEvent.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/input.h"

namespace SirEngine {
void EditorLayer::onAttach() {
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
    // if (globals::ENGINE_CONFIG->m_selectdedAdapterVendor ==
    //    ADAPTER_VENDOR::AMD) {
    style.Colors[ImGuiCol_WindowBg].w = 1.f;
    //  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.7f, 0.0f, 0.0f, 1.00f);
    //} else if (globals::ENGINE_CONFIG->m_selectdedAdapterVendor ==
    //           ADAPTER_VENDOR::NVIDIA) {
    //  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.4f, 0.0f, 0.75f);
    //  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.7f, 0.0f, 1.00f);
    //}
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
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void EditorLayer::onDetach() { ImGui_ImplDX12_Shutdown(); }

void EditorLayer::setupDockSpaceLayout(int width, int height) {
  if (ImGui::DockBuilderGetNode(dockIds.root) == NULL) {
    dockIds.root = ImGui::GetID("Root_Dockspace");

    ImGui::DockBuilderRemoveNode(dockIds.root);  // Clear out existing layout
    ImGui::DockBuilderAddNode(dockIds.root,
                              ImGuiDockNodeFlags_DockSpace);  // Add empty node
    ImGui::DockBuilderSetNodeSize(dockIds.root, ImVec2(width, height));

    dockIds.right = ImGui::DockBuilderSplitNode(dockIds.root, ImGuiDir_Right,
                                                0.2f, NULL, &dockIds.root);
    dockIds.left = ImGui::DockBuilderSplitNode(dockIds.root, ImGuiDir_Left,
                                               0.2f, NULL, &dockIds.root);
    dockIds.bottom = ImGui::DockBuilderSplitNode(dockIds.root, ImGuiDir_Down,
                                                 0.3f, NULL, &dockIds.root);

    ImGui::DockBuilderDockWindow("Edit Viewport", dockIds.root);
    ImGui::DockBuilderDockWindow("Play Viewport", dockIds.root);
    ImGui::DockBuilderDockWindow("Log", dockIds.bottom);
    ImGui::DockBuilderDockWindow("Asset Browser", dockIds.bottom);
    ImGui::DockBuilderDockWindow("Scene Hierarchy", dockIds.left);
    ImGui::DockBuilderDockWindow("Inspector", dockIds.right);
    ImGui::DockBuilderFinish(dockIds.root);
  }
}

void EditorLayer::onUpdate() {
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

  const ImVec2 pos{0, 0};
  ImGui::NewFrame();
  // ImGui::ShowDemoWindow();
  int width = globals::ENGINE_CONFIG->m_windowWidth;
  int height = globals::ENGINE_CONFIG->m_windowHeight;
  setupDockSpaceLayout(width, height);

  auto id = ImGui::GetID("Root_Dockspace");

  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

  ImGuiViewport *viewport = ImGui::GetMainViewport();

  ImGui::SetNextWindowPos(viewport->GetWorkPos());
  ImGui::SetNextWindowSize(viewport->GetWorkSize());
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    window_flags |= ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  ImGui::Begin("Editor", (bool *)0, window_flags);

  ImGui::PopStyleVar();
  ImGui::PopStyleVar(2);

  static bool showCamera = false;
  // shorcut_menu_bar();
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      ImGui::MenuItem("Open");
      ImGui::MenuItem("Save");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Settings")) {
      if (ImGui::BeginMenu("Docking")) {
        if (ImGui::MenuItem(
                "Flag: NoSplit", "",
                (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))
          dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
        if (ImGui::MenuItem(
                "Flag: NoResize", "",
                (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))
          dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
        if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "",
                            (dockspace_flags &
                             ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
          dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
        if (ImGui::MenuItem("Flag: PassthruCentralNode", "",
                            (dockspace_flags &
                             ImGuiDockNodeFlags_PassthruCentralNode) != 0))
          dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
        if (ImGui::MenuItem(
                "Flag: AutoHideTabBar", "",
                (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
          dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
        ImGui::EndMenu();
      }
      if (ImGui::Button("Camera Settings")) {
        showCamera = true;
      }
      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }

  ImGui::DockSpace(id, ImVec2(0.0f, 0.0f), dockspace_flags);

  ImGui::SetNextWindowDockID(dockIds.root, ImGuiCond_Appearing);
  ImGui::Begin("Viewport", (bool *)0);

  // if our viewport is hovered we set the flag, that will allow
  // our camera controller to behave properly
  bool isWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow);
  // storing the start of the cursor in the window before we add
  // the image, so we can use it to overlay imguizmo
  float x = ImGui::GetCursorScreenPos().x;
  float y = ImGui::GetCursorScreenPos().y;

  ImGui::End();

  ImGui::SetNextWindowDockID(dockIds.bottom, ImGuiCond_Appearing);
  ImGui::Begin("Assets", (bool *)0);
  ImGui::Text("assets go here");

  ImGui::End();

  ImGui::SetNextWindowDockID(dockIds.bottom, ImGuiCond_Appearing);
  ImGui::Begin("log", (bool *)0);
  // const std::string *buff = Log::getBuffer();
  // ImGui::Text("%s", buff->c_str());
  ImGui::End();

  ImGui::SetNextWindowDockID(dockIds.left, ImGuiCond_Appearing);
  bool m_showHierarchy = true;
  if (ImGui::Begin("Hierarchy", &m_showHierarchy)) {
    //  if (ImGui::Button("Clear Selection")) {
    //    CONTEXT->world.hierarchy.clearSelection();
    //  }
    //  m_hierarchy.render(&CONTEXT->world.hierarchy, &m_showHierarchy);

    ImGui::End();
  };

  ImGui::SetNextWindowDockID(dockIds.right, ImGuiCond_Appearing);
  ImGui::Begin("Inspector", (bool *)0);
  ImGui::Text("inspect values of selected objects here");

  ImGui::End();

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

#define SE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
void EditorLayer::onEvent(Event &event) {
  EventDispatcher dispatcher(event);
  dispatcher.dispatch<KeyTypeEvent>(
      SE_BIND_EVENT_FN(EditorLayer::onKeyTypeEvent));
  dispatcher.dispatch<MouseButtonPressEvent>(
      SE_BIND_EVENT_FN(EditorLayer::onMouseButtonPressEvent));
  dispatcher.dispatch<MouseButtonReleaseEvent>(
      SE_BIND_EVENT_FN(EditorLayer::onMouseButtonReleaseEvent));
  dispatcher.dispatch<MouseMoveEvent>(
      SE_BIND_EVENT_FN(EditorLayer::onMouseMoveEvent));
  dispatcher.dispatch<MouseScrollEvent>(
      SE_BIND_EVENT_FN(EditorLayer::onMouseScrolledEvent));
  dispatcher.dispatch<KeyboardPressEvent>(
      SE_BIND_EVENT_FN(EditorLayer::onKeyPressedEvent));
  dispatcher.dispatch<KeyboardReleaseEvent>(
      SE_BIND_EVENT_FN(EditorLayer::onKeyReleasedEvent));
  dispatcher.dispatch<WindowResizeEvent>(
      SE_BIND_EVENT_FN(EditorLayer::onWindowResizeEvent));
}
#undef SE_BIND_EVENT_FN

void EditorLayer::clear() {
#if BUILD_VK
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::VULKAN) {
    VK_CHECK(vkDeviceWaitIdle(vk::LOGICAL_DEVICE));
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
    vkDestroyRenderPass(vk::LOGICAL_DEVICE, imguiPass, nullptr);
  }
#endif
}

bool EditorLayer::onMouseButtonPressEvent(
    const MouseButtonPressEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.MouseDown[static_cast<int>(e.getMouseButton())] = true;
  globals::INPUT->m_uiCapturingMouse = io.WantCaptureMouse;
  return io.WantCaptureMouse;
}
bool EditorLayer::onMouseButtonReleaseEvent(
    const MouseButtonReleaseEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.MouseDown[static_cast<int>(e.getMouseButton())] = false;
  return false;
}
bool EditorLayer::onMouseMoveEvent(const MouseMoveEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.MousePos = ImVec2(e.getX(), e.getY());
  globals::INPUT->m_uiCapturingMouse = io.WantCaptureMouse;
  return io.WantCaptureMouse;
}
bool EditorLayer::onMouseScrolledEvent(const MouseScrollEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.MouseWheelH += e.getOffsetX();
  io.MouseWheel += e.getOffsetY();
  globals::INPUT->m_uiCapturingMouse = io.WantCaptureMouse;
  return io.WantCaptureMouse;
}
bool EditorLayer::onKeyPressedEvent(const KeyboardPressEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  int c = e.getKeyCode();
  io.KeysDown[c] = true;
  globals::INPUT->m_uiCapturingKeyboard = io.WantCaptureKeyboard;
  return io.WantCaptureKeyboard;
}
bool EditorLayer::onKeyReleasedEvent(const KeyboardReleaseEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.KeysDown[e.getKeyCode()] = false;
  globals::INPUT->m_uiCapturingKeyboard = io.WantCaptureKeyboard;
  return io.WantCaptureKeyboard;
}
bool EditorLayer::onWindowResizeEvent(const WindowResizeEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(static_cast<float>(e.getWidth()),
                          static_cast<float>(e.getHeight()));
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
  ImGui_ImplDX12_InvalidateDeviceObjects();

  return false;
}
bool EditorLayer::onKeyTypeEvent(const KeyTypeEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.AddInputCharacter(static_cast<uint16_t>(e.getKeyCode()));
  globals::INPUT->m_uiCapturingKeyboard = io.WantCaptureKeyboard;
  return io.WantCaptureKeyboard;
}

}  // namespace SirEngine
