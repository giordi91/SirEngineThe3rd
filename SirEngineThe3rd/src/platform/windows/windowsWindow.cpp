#include "SirEnginepch.h"

#include "SirEngine/events/appliacationEvent.h"
#include "SirEngine/log.h"
#include "platform/windows/windowsWindow.h"

namespace SirEngine {
// This needs to be implemented per platform
Window *Window::create(const WindowProps &props) {
  return new WindowsWindow(props);
};
WindowsWindow::WindowsWindow(const WindowProps &props) {

  SE_CORE_TRACE("Creating WindowsWindow with dimensions: {0}x{1}", props.width,
                props.height);

  WNDCLASSEX wc;
  DEVMODE dmScreenSettings;
  int posX, posY;

  // Get an external pointer to this object.
  windowsApplicationHandle = this;

  // Get the instance of this application.
  m_hinstance = GetModuleHandle(NULL);

  // Give the application a name.

  // Setup the windows class with default settings.
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc.lpfnWndProc = WndProc;
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
  //} else {
  // If windowed then set it to 800x600 resolution.
  // screenWidth =  constants->SCREEN_WIDTH;
  // screenHeight = constants->SCREEN_HEIGHT;
  m_data.width = props.width;
  m_data.height = props.height;
  m_data.title = props.title;

  // Place the window in the middle of the screen.
  posX = (GetSystemMetrics(SM_CXSCREEN) - m_data.width) / 2;
  posY = (GetSystemMetrics(SM_CYSCREEN) - m_data.height) / 2;
  //}

  // Create the window with the screen settings and get the handle to it.
  // m_hwnd = CreateWindowEx(
  //    WS_EX_APPWINDOW, m_applicationName, m_applicationName,
  //    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, posX, posY,
  //    screenWidth, screenHeight, NULL, NULL, m_hinstance, NULL);

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

  // if (m_input->IsKeyDown(VK_ESCAPE)) {
  //  done = true;
  //  continue;
  //}
  //// Otherwise do the frame processing.
  // if (m_graphics != nullptr) {
  //  result = m_graphics->frame();
  //  if (!result) {
  //    done = true;
  //  }
  //}
}

unsigned int WindowsWindow::getWidth() const { return m_data.width; }
unsigned int WindowsWindow::getHeight() const { return m_data.height; }
void WindowsWindow::setEventCallback(const EventCallbackFn &callback) {
  m_callback = callback;
}
void WindowsWindow::setVSync(bool ennabled) {}
void WindowsWindow::isVSync() const {}

LRESULT CALLBACK WindowsWindow::MessageHandler(HWND hwnd, UINT umsg,
                                               WPARAM wparam, LPARAM lparam) {

  /*
//if (m_ui_handler != nullptr) {
//  bool res = m_ui_handler(hwnd, umsg, wparam, lparam);
//  if (res) { return true; };
//}
if (ImGui_ImplWin32_WndProcHandler(hwnd, umsg, wparam, lparam)) {
return true;
}*/

  switch (umsg) {

  case WM_QUIT: {
    int x = 0;
    return 0;
  }
  case WM_CLOSE: {

    WindowCloseEvent closeEvent;
    m_callback(closeEvent);
    int y = 0;
  }

  case WM_SIZE: {
    //  std::cout << "resizing" << std::endl;
    // if (m_graphics != nullptr && (m_graphics->m_Direct3D!= nullptr) &&
    // m_graphics->m_Direct3D->getDevice()!= NULL && wparam != SIZE_MINIMIZED)
    //{
    //    ImGui_ImplDX11_InvalidateDeviceObjects();

    //	auto* render = rendering::RenderingManager::get_instance();
    //	render->m_screenWidth= (UINT)LOWORD(lparam);
    //	render->m_screenHeight= (UINT)HIWORD(lparam);
    //    m_graphics->m_Direct3D->resize(render->m_screenWidth,render->m_screenHeight);

    //	/*
    //	auto* deferred = deferred::DeferredTargets::get_instance();
    //	if (render->m_screenWidth != -1 && render->m_screenHeight != 1)
    //	{
    //		deferred->resize(render->m_screenWidth, render->m_screenHeight);
    //	}
    //	*/
    //    //m_graphics->m_Direct3D->m_swapChain->ResizeBuffers(0,
    //    //(UINT)LOWORD(lparam), (UINT)HIWORD(lparam), DXGI_FORMAT_UNKNOWN, 0);
    //
    //    ////m_Graphics->m_Direct3D->initialize( (UINT)LOWORD(lparam),
    //    //(UINT)HIWORD(lparam),true,hwnd,false,0.0f,1.0f);
    //    ImGui_ImplDX11_CreateDeviceObjects();
    //}
    return 0;
  }
    // Check if a key has been pressed on the keyboard.
  case WM_KEYDOWN: {
    //// If a key is pressed send it to the input object so it can record that
    // state.
    // m_input->KeyDown((unsigned int)wparam);
    return 0;
  }

    // Check if a key has been released on the keyboard.
  case WM_KEYUP: {
    // If a key is released then send it to the input object so it can unset the
    // state for that key.
    // m_input->KeyUp((unsigned int)wparam);
    return 0;
  }
  case WM_LBUTTONDOWN: {
    // m_input->m_mouse[0] = 1;
    // m_input->m_mouse[2] = 0;
    return 0;
  }
  case WM_RBUTTONDOWN: {
    // m_input->m_mouse[1] = 1;
    // m_input->m_mouse[2] = 0;
    return 0;
  }
  case WM_MBUTTONDOWN: {
    // m_input->m_mouse[3] = 1;
    // m_input->m_mouse[2] = 0;
    return 0;
  }
  case WM_MOUSEWHEEL: {
    // bool forward = (int)GET_WHEEL_DELTA_WPARAM(wparam) > 0;
    // m_input->m_mouse[2] = forward ? 1 : -1;
    return 0;
  }
  case WM_LBUTTONUP: {
    // m_input->selection = true;
    // m_input->m_mouse[0] = 0;
    // m_input->m_mouse[2] = 0;
    return 0;
  }
  case WM_RBUTTONUP: {
    // m_input->m_mouse[1] = 0;
    // m_input->m_mouse[2] = 0;
    return 0;
  }
  case WM_MBUTTONUP: {
    // m_input->m_mouse[3] = 0;
    // m_input->m_mouse[2] = 0;
    return 0;
  }
  case WM_MOUSEMOVE: {
    // m_input->m_mouse_posX = GET_X_LPARAM(lparam);
    // m_input->m_mouse_posY = GET_Y_LPARAM(lparam);
    return 0;
  }

    // Any other messages send to the default message handler as our application
    // won't make use of them.
  default: {
    return DefWindowProc(hwnd, umsg, wparam, lparam);
  }
  }
}

} // namespace SirEngine

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam,
                         LPARAM lparam) {
  switch (umessage) {
    // Check if the window is being destroyed.
    // case WM_DESTROY: {
    //  PostQuitMessage(0);
    //  return 0;
    //}

    //  // Check if the window is being closed.
    // case WM_CLOSE: {
    //  PostQuitMessage(0);
    //  return 0;
    //}

    // All other messages pass to the message handler in the system class.
  default: {
    return windowsApplicationHandle->MessageHandler(hwnd, umessage, wparam,
                                                    lparam);
  }
  }
}
