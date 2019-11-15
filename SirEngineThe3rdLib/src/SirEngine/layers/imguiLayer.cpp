#include <Windows.h>

#include "SirEngine/core.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "imgui/imgui.h"
#include "imguiLayer.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/imgui_impl_dx12.h"

#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/swapChain.h"

#include "SirEngine/application.h"
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
  // need to initialize ImGui dx12
  dx12::DescriptorPair pair;
  dx12::GLOBAL_CBV_SRV_UAV_HEAP->reserveDescriptor(pair);

  ImGui_ImplDX12_Init(dx12::DEVICE, FRAME_BUFFERS_COUNT,
                      DXGI_FORMAT_R8G8B8A8_UNORM, pair.cpuHandle,
                      pair.gpuHandle);

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

  io.DisplaySize = ImVec2(static_cast<float>(globals::ENGINE_CONFIG->m_windowWidth),
                          static_cast<float>(globals::ENGINE_CONFIG->m_windowHeight));

  m_renderGraph.initialize(dx12::RENDERING_GRAPH);
  m_shaderWidget.initialize();
}

void ImguiLayer::onDetach() { ImGui_ImplDX12_Shutdown(); }

void ImguiLayer::onUpdate() {

  if (!m_shouldShow) {
    return;
  }

  annotateGraphicsBegin("UI Draw");
  TextureHandle destination = dx12::SWAP_CHAIN->currentBackBufferTexture();
  D3D12_RESOURCE_BARRIER barriers[1];
  int counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      destination, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, 0);
  if (counter) {
    dx12::CURRENT_FRAME_RESOURCE->fc.commandList->ResourceBarrier(counter,
                                                                  barriers);
  }

  globals::TEXTURE_MANAGER->bindBackBuffer(false);

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

  ImGui_ImplDX12_NewFrame();

  IM_ASSERT(io.Fonts->IsBuilt() &&
            "Font atlas not built! It is generally built by the renderer "
            "back-end. Missing call to renderer _NewFrame() function? e.g. "
            "ImGui_ImplOpenGL3_NewFrame().");

  bool show_demo_window = true;

  const ImVec2 pos{0, 0};
  ImGui::NewFrame();
  // ImGui::ShowDemoWindow(&show_demo_window);

  ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
  ImGui::Begin("Debug", &show_demo_window, ImGuiWindowFlags_MenuBar);
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Tools")) {
      if (ImGui::MenuItem("Shader compiler", nullptr)) {
        m_renderShaderCompiler = !m_renderShaderCompiler;
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

  ImGui::End();

  ImGui::Render();
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
                                dx12::CURRENT_FRAME_RESOURCE->fc.commandList);
  annotateGraphicsEnd();
}

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
bool ImguiLayer::onMouseButtonPressEvent(const MouseButtonPressEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.MouseDown[static_cast<int>(e.getMouseButton())] = true;
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
  return io.WantCaptureMouse;
}
bool ImguiLayer::onMouseScrolledEvent(const MouseScrollEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.MouseWheelH += e.getOffsetX();
  io.MouseWheel += e.getOffsetY();
  return io.WantCaptureMouse;
}
bool ImguiLayer::onKeyPressedEvent(const KeyboardPressEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  int c = e.getKeyCode();
  io.KeysDown[c] = true;
  if (c == TRIGGER_UI_BUTTON) {
    m_shouldShow = !m_shouldShow;
  }
  return io.WantCaptureKeyboard;
}
bool ImguiLayer::onKeyReleasedEvent(const KeyboardReleaseEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.KeysDown[e.getKeyCode()] = false;
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
  return io.WantCaptureKeyboard;
}

bool ImguiLayer::onRenderGraphEvent(const RenderGraphChanged &) {
  m_renderGraph.initialize(dx12::RENDERING_GRAPH);
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
} // namespace SirEngine
