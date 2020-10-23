
#include "SirEngine/Layers/imguiDebugLayer.h"

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
#include "SirEngine/events/applicationEvent.h"
#include "SirEngine/events/event.h"
#include "SirEngine/events/keyboardEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/events/renderGraphEvent.h"
#include "SirEngine/events/scriptingEvent.h"
#include "SirEngine/events/shaderCompileEvent.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/input.h"
#include "SirEngine/ui/imguiManager.h"

namespace SirEngine {
void ImguiLayer::onAttach() {
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

  globals::IMGUI_MANAGER->startFrame();

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
        m_renderGrassConfig = !m_renderGrassConfig;
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

  globals::IMGUI_MANAGER->endFrame();
}

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
  globals::INPUT->m_uiCapturingMouse = io.WantCaptureMouse;
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
  globals::INPUT->m_uiCapturingMouse = io.WantCaptureMouse;
  return io.WantCaptureMouse;
}
bool ImguiLayer::onMouseScrolledEvent(const MouseScrollEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.MouseWheelH += e.getOffsetX();
  io.MouseWheel += e.getOffsetY();
  globals::INPUT->m_uiCapturingMouse = io.WantCaptureMouse;
  return io.WantCaptureMouse;
}
bool ImguiLayer::onKeyPressedEvent(const KeyboardPressEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  int c = e.getKeyCode();
  io.KeysDown[c] = true;
  if (c == TRIGGER_UI_BUTTON) {
    m_shouldShow = !m_shouldShow;
  }
  globals::INPUT->m_uiCapturingKeyboard = io.WantCaptureKeyboard;
  return io.WantCaptureKeyboard;
}
bool ImguiLayer::onKeyReleasedEvent(const KeyboardReleaseEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.KeysDown[e.getKeyCode()] = false;
  globals::INPUT->m_uiCapturingKeyboard = io.WantCaptureKeyboard;
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
  globals::INPUT->m_uiCapturingKeyboard = io.WantCaptureKeyboard;
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
