#include <Windows.h>

#include "platform/windows/windowsWindow.h"

#include "SirEngine/events/applicationEvent.h"
#include "SirEngine/events/keyboardEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/log.h"

#include <windowsx.h>

#include "SirEngine/events/scriptingEvent.h"
#include "SirEngine/events/shaderCompileEvent.h"
#include "SirEngine/input.h"
#include "SirEngine/scripting/scriptingContext.h"
#include "platform/windows/graphics/dx12/dx12SwapChain.h"

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
  case WM_CHAR: {
    // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
    if (wparam > 0 && wparam < 0x10000) {

      // basic shortcut handling
      // 114 is r, we want to reload/recompile shader
      auto code = static_cast<uint32_t>(wparam);
      if (code == 114) {
        RequestShaderCompileEvent e;
        ASSERT_CALLBACK_AND_DISPATCH(e);
        return 0;
      } else if (code == 93) {
        // 93 is ], used to reload scripts
        ReloadScriptsEvent e;
        ASSERT_CALLBACK_AND_DISPATCH(e);
        return 0;
      }

      globals::INPUT->keyDown(code);
      KeyTypeEvent e{static_cast<uint32_t>(wparam)};
      ASSERT_CALLBACK_AND_DISPATCH(e);
    }
    return 0;
  }

  case WM_SIZE: {
    // the reason for this check is because the window call a resize immediately
    // before the user has time to set the callback to the window if
    auto callback = windowsApplicationHandle->getEventCallback();
    if (callback != nullptr) {
      auto w = static_cast<uint32_t>(LOWORD(lparam));
      auto h = static_cast<uint32_t>(HIWORD(lparam));
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

    auto code = static_cast<uint32_t>(wparam);
    globals::INPUT->keyDown(code);
    KeyboardPressEvent e{code};
    ASSERT_CALLBACK_AND_DISPATCH(e);

    return 0;
  }

    // Check if a key has been released on the keyboard.
  case WM_KEYUP: {
    auto code = static_cast<uint32_t>(wparam);
    globals::INPUT->keyUp(code);
    KeyboardReleaseEvent e{code};

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
    globals::INPUT->setMouse(MOUSE_BUTTONS::LEFT, 1);
    MouseButtonPressEvent e{MOUSE_BUTTONS_EVENT::LEFT};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_RBUTTONDOWN: {
    globals::INPUT->setMouse(MOUSE_BUTTONS::RIGHT, 1);
    MouseButtonPressEvent e{MOUSE_BUTTONS_EVENT::RIGHT};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_MBUTTONDOWN: {
    globals::INPUT->setMouse(MOUSE_BUTTONS::MIDDLE, 1);
    MouseButtonPressEvent e{MOUSE_BUTTONS_EVENT::MIDDLE};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_MOUSEWHEEL: {
    auto movementY = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam));
    globals::INPUT->setMouse(MOUSE_BUTTONS::WHEEL, movementY ? 1 : -1);
    // side tilt of the scroll currently not supported, always 0.0f
    MouseScrollEvent e{0.0f, movementY};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_LBUTTONUP: {
    globals::INPUT->setMouse(MOUSE_BUTTONS::LEFT, 0);
    MouseButtonReleaseEvent e{MOUSE_BUTTONS_EVENT::LEFT};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_RBUTTONUP: {
    globals::INPUT->setMouse(MOUSE_BUTTONS::RIGHT, 0);
    MouseButtonReleaseEvent e{MOUSE_BUTTONS_EVENT::RIGHT};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_MBUTTONUP: {
    globals::INPUT->setMouse(MOUSE_BUTTONS::MIDDLE, 0);
    MouseButtonReleaseEvent e{MOUSE_BUTTONS_EVENT::MIDDLE};
    ASSERT_CALLBACK_AND_DISPATCH(e);
    return 0;
  }
  case WM_MOUSEMOVE: {
    const auto posX = static_cast<float>(GET_X_LPARAM(lparam));
    const auto posY = static_cast<float>(GET_Y_LPARAM(lparam));
    globals::INPUT->setMousePos(posX, posY);

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
BaseWindow *BaseWindow::create(const WindowProps &props) {
  return new WindowsWindow(props);
};
WindowsWindow::WindowsWindow(const WindowProps &props) {

  SE_CORE_TRACE("Creating WindowsWindow with dimensions: {0}x{1}", props.width,
                props.height);

  WNDCLASSEX wc;

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
  wc.lpszClassName = frameConvertWide(props.title);
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
  // uint32_t posX = (GetSystemMetrics(SM_CXSCREEN) - m_data.width) / 2u;
  // uint32_t posY = (GetSystemMetrics(SM_CYSCREEN) - m_data.height) / 2u;
  auto* title = wc.lpszClassName;

  constexpr DWORD style =
      WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  RECT wr{0, 0, static_cast<LONG>(m_data.width),
          static_cast<LONG>(m_data.height)};
  // needed to create the window of the right size, or wont match the gui
  AdjustWindowRectEx(&wr, style, false, NULL);
  m_hwnd = CreateWindowEx(0, title, title, style, 0, 0,
                          wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr,
                          GetModuleHandle(nullptr), 0);

  // Bring the window up on the screen and set it as main focus.
  ShowWindow(m_hwnd, SW_SHOWDEFAULT);
  UpdateWindow(m_hwnd);
  SetForegroundWindow(m_hwnd);
  SetFocus(m_hwnd);

  // Hide the mouse cursor.
  ShowCursor(true);

  //update the native window
  assert(sizeof(m_hinstance) == 8);
  memcpy(&m_nativeWindow.data, &m_hinstance, sizeof(m_hinstance)); 
  assert(sizeof(m_hwnd) == 8);
  memcpy(&m_nativeWindow.data2, &m_hwnd, sizeof(m_hwnd)); 

  // TODO have a centralize initialize for the engine
  globals::SCRIPTING_CONTEXT = new ScriptingContext();
  globals::SCRIPTING_CONTEXT->init();

  globals::INPUT = new Input();
  globals::INPUT->init();
}

// TODO either remove it or move the render stuff in here, what should go in
// onUpdate? what should go on render?
void WindowsWindow::render() {}

void WindowsWindow::onUpdate() {

  MSG msg;
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

void WindowsWindow::onResize(uint32_t width, uint32_t height) {
  m_data.width = width;
  m_data.height = height;
}

uint32_t WindowsWindow::getWidth() const { return m_data.width; }
uint32_t WindowsWindow::getHeight() const { return m_data.height; }
void WindowsWindow::setEventCallback(const EventCallbackFn &callback) {
  m_callback = callback;
}
void WindowsWindow::setVSync(bool) { assert(0 && "not implemented yet"); }
void WindowsWindow::isVSync() const { assert(0 && "not implemented yet"); }

} // namespace SirEngine
