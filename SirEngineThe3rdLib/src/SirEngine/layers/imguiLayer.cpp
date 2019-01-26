#include <Windows.h>

#include "SirEngine/core.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "imgui/imgui.h"
#include "imguiLayer.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/imgui_impl_dx12.h"

namespace SirEngine {
void ImguiLayer::onAttach() {
  // need to initialize ImGui dx12
  assert(m_fontTextureDescriptor == nullptr);
  m_fontTextureDescriptor = new dx12::D3DBuffer();
  m_descriptorIndex =
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->reserveDescriptor(m_fontTextureDescriptor);

  ImGui_ImplDX12_Init(dx12::DEVICE, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
                      m_fontTextureDescriptor->cpuDescriptorHandle,
                      m_fontTextureDescriptor->gpuDescriptorHandle);

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

  ::QueryPerformanceFrequency((LARGE_INTEGER *)&g_TicksPerSecond);
  ::QueryPerformanceCounter((LARGE_INTEGER *)&g_Time);

  io.DisplaySize = ImVec2(Globals::SCREEN_WIDTH, Globals::SCREEN_HEIGHT);
}

void ImguiLayer::onDetach() {
  ImGui_ImplDX12_Shutdown();
  delete m_fontTextureDescriptor;
  m_fontTextureDescriptor == nullptr;
}

void ImguiLayer::onUpdate() {

  // Read keyboard modifiers inputs
  ImGuiIO &io = ImGui::GetIO();

  // Setup time step
  INT64 current_time;
  ::QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
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
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  ImGui::NewFrame();
  ImGui::ShowDemoWindow(&show_demo_window);
  static float f = 0.0f;
  static int counter = 0;

  ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and
                                 // append into it.

  ImGui::Text("This is some useful text."); // Display some text (you can use
                                            // a format strings too)
  ImGui::Checkbox(
      "Demo Window",
      &show_demo_window); // Edit bools storing our window open/close state
  ImGui::Checkbox("Another Window", &show_demo_window);

  ImGui::SliderFloat("float", &f, 0.0f,
                     1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
  ImGui::ColorEdit3(
      "clear color",
      (float *)&clear_color); // Edit 3 floats representing a color

  if (ImGui::Button("Button")) // Buttons return true when clicked (most
                               // widgets return true when edited/activated)
    counter++;
  ImGui::SameLine();
  ImGui::Text("counter = %d", counter);

  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::End();

  ImGui::Render();
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
                                dx12::CURRENT_FRAME_RESOURCE->fc.commandList);
}

void ImguiLayer::onEvent(Event &event) {

  EventDispatcher dispatcher(event);
  dispatcher.dispatch<KeyTypeEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::OnKeyTypeEvent));
  dispatcher.dispatch<MouseButtonPressEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::OnMouseButtonPressEvent));
  dispatcher.dispatch<MouseButtonReleaseEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::OnMouseButtonReleaseEvent));
  dispatcher.dispatch<MouseMoveEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::OnMouseMoveEvent));
  dispatcher.dispatch<MouseScrollEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::OnMouseScrolledEvent));
  dispatcher.dispatch<KeyboardPressEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::OnKeyPressedEvent));
  dispatcher.dispatch<KeyboardReleaseEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::OnKeyReleasedEvent));
  dispatcher.dispatch<WindowResizeEvent>(
      SE_BIND_EVENT_FN(ImguiLayer::OnWindowResizeEvent));
}
bool ImguiLayer::OnMouseButtonPressEvent(MouseButtonPressEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  io.MouseDown[static_cast<int>(e.getMouseButton())] = true;
  return io.WantCaptureMouse;
}
bool ImguiLayer::OnMouseButtonReleaseEvent(MouseButtonReleaseEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  io.MouseDown[static_cast<int>(e.getMouseButton())] = false;
  return false;
}
bool ImguiLayer::OnMouseMoveEvent(MouseMoveEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  io.MousePos = ImVec2(e.getX(), e.getY());
  return io.WantCaptureMouse;
}
bool ImguiLayer::OnMouseScrolledEvent(MouseScrollEvent &e) {

  ImGuiIO &io = ImGui::GetIO();
  io.MouseWheelH += e.getOffsetX();
  io.MouseWheel += e.getOffsetY();
  return io.WantCaptureMouse;
}

bool ImguiLayer::OnKeyPressedEvent(KeyboardPressEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  int c = e.getKeyCode();
  io.KeysDown[c] = true;
  return io.WantCaptureKeyboard;
}
bool ImguiLayer::OnKeyReleasedEvent(KeyboardReleaseEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  io.KeysDown[e.getKeyCode()] = false;
  return io.WantCaptureKeyboard;
}
bool ImguiLayer::OnWindowResizeEvent(WindowResizeEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(e.getWidth(), e.getHeight());
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
  ImGui_ImplDX12_InvalidateDeviceObjects();

  return false;
}
bool ImguiLayer::OnKeyTypeEvent(KeyTypeEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  io.AddInputCharacter((unsigned short)e.getKeyCode());
  return io.WantCaptureKeyboard;
}
} // namespace SirEngine
