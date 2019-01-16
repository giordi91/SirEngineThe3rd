
#include "platform/windows/windowsWindow.h"
#include "platform/windows/graphics/dx12/DX12.h"

#include "SirEngine/events/appliacationEvent.h"
#include "SirEngine/events/keyboardEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/log.h"

#include "platform/windows/graphics/dx12/barrierUtils.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/swapChain.h"

#include "imgui/imgui.h"
#include "platform/windows/graphics/dx12/imgui_impl_dx12.h"

#include <windowsx.h>

namespace SirEngine {

// specific windows implementation, most notably window proc
// and message pump handle
namespace WindowsImpl {
static SirEngine::WindowsWindow *windowsApplicationHandle = nullptr;
LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam,
                         LPARAM lparam) {

#define ASSERT_CALLBACK_AND_DISPATCH(e)                                        \
  auto callback = windowsApplicationHandle->getEventCallback();                \
  assert(callback != nullptr);                                                 \
  callback(e);

  switch (umessage) {

  case WM_QUIT: {
    WindowCloseEvent closeEvent;
    ASSERT_CALLBACK_AND_DISPATCH(closeEvent);
    return 0;
  }
  case WM_CLOSE: {
    WindowCloseEvent closeEvent;
    ASSERT_CALLBACK_AND_DISPATCH(closeEvent);
    return 0;
  }

  case WM_SIZE: {
    // the reason for this check is because the window call a resize immediately
    // before the user has time to set the callback to the window if
    auto callback = windowsApplicationHandle->getEventCallback();
    if (callback != nullptr) {
      unsigned int w = (UINT)LOWORD(lparam);
      unsigned int h = (UINT)HIWORD(lparam);
      WindowResizeEvent resizeEvent{w, h};
      callback(resizeEvent);
    }
    return 0;
  }
    // Check if a key has been pressed on the keyboard.
  case WM_KEYDOWN: {
    // repeated key message not supported as differentiator for now,
    // if I wanted to do that seems like bit 30 of lparam is the one
    // giving you if the first press or a repeat, not sure how to get the
    //"lag" in before sending repeats.
    KeyboardPressEvent e{static_cast<unsigned int>(wparam)};
    ASSERT_CALLBACK_AND_DISPATCH(e);

    return 0;
  }

    // Check if a key has been released on the keyboard.
  case WM_KEYUP: {
    KeyboardReleaseEvent e{static_cast<unsigned int>(wparam)};

#ifdef QUIT_ESCAPE
    // here we hard-coded this behavior where if the VK_ESCAPE button
    // is pressed I want the message to be sent out as close window,
    // this is a personal preference
    if (wparam == VK_ESCAPE) {
      WindowCloseEvent closeEvent;
      ASSERT_CALLBACK_AND_DISPATCH(closeEvent);
      return 0;
    }
#endif

    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_LBUTTONDOWN: {
    MouseButtonPressEvent e{MOUSE_BUTTONS_EVENT::LEFT};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_RBUTTONDOWN: {
    MouseButtonPressEvent e{MOUSE_BUTTONS_EVENT::RIGHT};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_MBUTTONDOWN: {
    MouseButtonPressEvent e{MOUSE_BUTTONS_EVENT::MIDDLE};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_MOUSEWHEEL: {
    float movementX = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam));
    // side tilt of the scroll currently not supported, always 0.0f
    MouseScrollEvent e{movementX, 0.0f};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_LBUTTONUP: {
    MouseButtonReleaseEvent e{MOUSE_BUTTONS_EVENT::LEFT};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_RBUTTONUP: {
    MouseButtonReleaseEvent e{MOUSE_BUTTONS_EVENT::RIGHT};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_MBUTTONUP: {
    MouseButtonReleaseEvent e{MOUSE_BUTTONS_EVENT::MIDDLE};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_MOUSEMOVE: {
    float posX = static_cast<float>(GET_X_LPARAM(lparam));
    float posY = static_cast<float>(GET_Y_LPARAM(lparam));
    MouseMoveEvent e{posX, posY};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }

  // Any other messages send to the default message handler as our application
  // won't make use of them.
  default: {
    return DefWindowProc(hwnd, umessage, wparam, lparam);
  }
  }
}
} // namespace WindowsImpl

// This needs to be implemented per platform
Window *Window::create(const WindowProps &props) {
  return new WindowsWindow(props);
};
WindowsWindow::WindowsWindow(const WindowProps &props) {

  SE_CORE_TRACE("Creating WindowsWindow with dimensions: {0}x{1}", props.width,
                props.height);

  WNDCLASSEX wc;
  // DEVMODE dmScreenSettings;
  int posX, posY;

  // Get an external pointer to this object.
  WindowsImpl::windowsApplicationHandle = this;

  // Get the instance of this application.
  m_hinstance = GetModuleHandle(NULL);

  // Give the application a name.

  // Setup the windows class with default settings.
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc.lpfnWndProc = WindowsImpl::WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = m_hinstance;
  wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
  wc.hIconSm = wc.hIcon;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszMenuName = NULL;
  std::wstring title(props.title.begin(), props.title.end());
  wc.lpszClassName = title.c_str();
  wc.cbSize = sizeof(WNDCLASSEX);

  // Register the window class.
  RegisterClassEx(&wc);

  // stuff for full-screen not supported yet
  // Determine the resolution of the clients desktop screen.
  // Setup the screen settings depending on whether it is running in full screen
  // or in windowed mode.
  // if (constants->FULL_SCREEN) {
  //  // If full screen set the screen to maximum size of the users desktop and
  //  // 32bit.
  //  memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
  //  dmScreenSettings.dmSize = sizeof(dmScreenSettings);
  //  dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
  //  dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
  //  dmScreenSettings.dmBitsPerPel = 32;
  //  dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

  //  // Change the display settings to full screen.
  //  ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

  // screenWidth = GetSystemMetrics(SM_CXSCREEN);
  // screenHeight = GetSystemMetrics(SM_CYSCREEN);
  //  // Set the position of the window to the top left corner.
  //  posX = posY = 0;
  m_data.width = props.width;
  m_data.height = props.height;
  m_data.title = props.title;

  // Place the window in the middle of the screen.
  posX = (GetSystemMetrics(SM_CXSCREEN) - m_data.width) / 2;
  posY = (GetSystemMetrics(SM_CYSCREEN) - m_data.height) / 2;

  constexpr DWORD style =
      WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  RECT wr{0, 0, (LONG)m_data.width, (LONG)m_data.height};
  // needed to create the window of the right size, or wont match the gui
  AdjustWindowRectEx(&wr, style, false, NULL);
  m_hwnd = CreateWindowEx(0, title.c_str(), title.c_str(), style, 0, 0,
                          wr.right - wr.left, wr.bottom - wr.top, NULL, NULL,
                          GetModuleHandle(NULL), 0);

  // Bring the window up on the screen and set it as main focus.
  ShowWindow(m_hwnd, SW_SHOWDEFAULT);
  UpdateWindow(m_hwnd);
  SetForegroundWindow(m_hwnd);
  SetFocus(m_hwnd);

  // Hide the mouse cursor.
  ShowCursor(true);

  // initialize dx12
  bool result = dx12::initializeGraphics();
  if (!result) {
    SE_CORE_ERROR("FATAL: could not initialize graphics");
  }
  dx12::DX12Handles::swapChain = new dx12::SwapChain();
  dx12::DX12Handles::swapChain->initialize(m_hwnd, m_data.width, m_data.height);
  dx12::flushCommandQueue(dx12::DX12Handles::commandQueue);
  dx12::DX12Handles::swapChain->resize(dx12::DX12Handles::commandList,
                                       m_data.width, m_data.height);

  dx12::D3DBuffer *m_fontTextureDescriptor = nullptr;
  int m_descriptorIndex;
  assert(m_fontTextureDescriptor == nullptr);
  m_fontTextureDescriptor = new dx12::D3DBuffer();
  m_descriptorIndex = dx12::DX12Handles::globalCBVSRVUAVheap->reserveDescriptor(
      m_fontTextureDescriptor);

  ImGui_ImplDX12_Init(dx12::DX12Handles::device, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
                      m_fontTextureDescriptor->cpuDescriptorHandle,
                      m_fontTextureDescriptor->gpuDescriptorHandle);
}

void WindowsWindow::render() {
  // Clear the back buffer and depth buffer.
  float gray[4] = {0.5f, 0.9f, 0.5f, 1.0f};
  // Reuse the memory associated with command recording.
  // We can only reset when the associated command lists have finished execution
  // on the GPU.
  resetAllocatorAndList(dx12::DX12Handles::commandList);
  // Indicate a state transition on the resource usage.
  auto *commandList = dx12::DX12Handles::commandList->commandList;
  D3D12_RESOURCE_BARRIER rtbarrier[1];

  int rtcounter = dx12::transitionTexture2DifNeeded(
      dx12::DX12Handles::swapChain->currentBackBuffer(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, rtbarrier, 0);
  if (rtcounter != 0) {
    commandList->ResourceBarrier(rtcounter, rtbarrier);
  }

  // Set the viewport and scissor rect.  This needs to be reset whenever the
  // command list is reset.
  commandList->RSSetViewports(1, dx12::DX12Handles::swapChain->getViewport());
  commandList->RSSetScissorRects(
      1, dx12::DX12Handles::swapChain->getScissorRect());

  // Clear the back buffer and depth buffer.
  commandList->ClearRenderTargetView(
      dx12::DX12Handles::swapChain->currentBackBufferView(), gray, 0, nullptr);

  dx12::DX12Handles::swapChain->clearDepth();
  dx12::SwapChain *swapChain = dx12::DX12Handles::swapChain;
  // Specify the buffers we are going to render to.
  auto back = swapChain->currentBackBufferView();
  auto depth = swapChain->getDepthCPUDescriptor();
  commandList->OMSetRenderTargets(1, &back, true, &depth);
  //&m_depthStencilBufferResource.cpuDescriptorHandle);
  auto* heap = dx12::DX12Handles::globalCBVSRVUAVheap->getResource();
  commandList->SetDescriptorHeaps(1, &heap);



  ImGui_ImplDX12_NewFrame();

  ImGuiIO &io = ImGui::GetIO();
  IM_ASSERT(io.Fonts->IsBuilt() &&
            "Font atlas not built! It is generally built by the renderer "
            "back-end. Missing call to renderer _NewFrame() function? e.g. "
            "ImGui_ImplOpenGL3_NewFrame().");

  io.DisplaySize = ImVec2((float)(1280), (float)(720));

  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  ImGui::NewFrame();
  // ImGui::ShowDemoWindow(&show_demo_window);
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

  // Rendering
  // FrameContext *frameCtxt = WaitForNextFrameResources();
  // UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
  // frameCtxt->CommandAllocator->Reset();

  // D3D12_RESOURCE_BARRIER barrier = {};
  // barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  // barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  // barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
  // barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  // barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  // barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

  // g_pd3dCommandList->Reset(frameCtxt->CommandAllocator, NULL);
  // g_pd3dCommandList->ResourceBarrier(1, &barrier);
  // g_pd3dCommandList->ClearRenderTargetView(
  //    g_mainRenderTargetDescriptor[backBufferIdx], (float *)&clear_color, 0,
  //    NULL);
  // g_pd3dCommandList->OMSetRenderTargets(
  //    1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
  // g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
  ImGui::Render();
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
                                dx12::DX12Handles::commandList->commandList);
  // barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  // barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  // g_pd3dCommandList->ResourceBarrier(1, &barrier);
  // g_pd3dCommandList->Close();

  // g_pd3dCommandQueue->ExecuteCommandLists(
  //    1, (ID3D12CommandList *const *)&g_pd3dCommandList);

  // g_pSwapChain->Present(1, 0); // Present with vsync
  //// g_pSwapChain->Present(0, 0); // Present without vsync

  // UINT64 fenceValue = g_fenceLastSignaledValue + 1;
  // g_pd3dCommandQueue->Signal(g_fence, fenceValue);
  // g_fenceLastSignaledValue = fenceValue;
  // frameCtxt->FenceValue = fenceValue;

  // finally transition the resource to be present
  rtcounter =
      dx12::transitionTexture2D(swapChain->currentBackBuffer(),
                                D3D12_RESOURCE_STATE_PRESENT, rtbarrier, 0);
  commandList->ResourceBarrier(rtcounter, rtbarrier);

  // Done recording commands.
  dx12::executeCommandList(dx12::DX12Handles::commandQueue,
                           dx12::DX12Handles::commandList);

  // swap the back and front buffers
  swapChain->present();

  // Wait until frame commands are complete.  This waiting is inefficient and is
  // done for simplicity.  Later we will show how to organize our rendering code
  // so we do not have to wait per frame.
  dx12::flushCommandQueue(dx12::DX12Handles::commandQueue);
}

void WindowsWindow::OnUpdate() {

  MSG msg;
  bool done = false;
  bool result = true;

  // initialize the message structure.
  ZeroMemory(&msg, sizeof(MSG));

  // Loop until there is a quit message from the window or the user.
  // Handle the windows messages.
  if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // do render
  render();
}

unsigned int WindowsWindow::getWidth() const { return m_data.width; }
unsigned int WindowsWindow::getHeight() const { return m_data.height; }
void WindowsWindow::setEventCallback(const EventCallbackFn &callback) {
  m_callback = callback;
}
void WindowsWindow::setVSync(bool ennabled) {
  assert(0 && "not implemented yet");
}
void WindowsWindow::isVSync() const { assert(0 && "not implemented yet"); }

} // namespace SirEngine
