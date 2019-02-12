#include <Windows.h>

#include "SirEngine/core.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "imgui/imgui.h"
#include "imguiLayer.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/imgui_impl_dx12.h"

#include "SirEngine/graphics/nodes/FinalBlitNode.h"
#include "SirEngine/graphics/nodes/assetManagerNode.h"
#include "SirEngine/graphics/nodes/simpleForward.h"

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

  io.DisplaySize = ImVec2(static_cast<float>(globals::SCREEN_WIDTH),
                          static_cast<float>(globals::SCREEN_HEIGHT));

  m_renderGraph.initialize(dx12::RENDERING_GRAPH);
}

void ImguiLayer::onDetach() { ImGui_ImplDX12_Shutdown(); }

void ImguiLayer::onUpdate() {

  if (!m_shouldShow) {
    return;
  }

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
  ImGui::NewFrame();
  ImGui::ShowDemoWindow(&show_demo_window);

  ImGui::Begin("Performance");
  m_frameTimings.render();
  m_memoryUsage.render();
  m_renderGraph.render();
  // bool s =true;
  // ShowExampleAppCustomNodeGraph(&s);
  ImGui::End();

  ImGui::Render();
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
                                dx12::CURRENT_FRAME_RESOURCE->fc.commandList);
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
  io.DisplaySize = ImVec2(e.getWidth(), e.getHeight());
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
  ImGui_ImplDX12_InvalidateDeviceObjects();

  return false;
}
bool ImguiLayer::onKeyTypeEvent(const KeyTypeEvent &e) const {
  ImGuiIO &io = ImGui::GetIO();
  io.AddInputCharacter(static_cast<uint16_t>(e.getKeyCode()));
  return io.WantCaptureKeyboard;
}
} // namespace SirEngine
