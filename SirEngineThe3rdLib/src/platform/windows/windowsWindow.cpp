#include <Windows.h>

#include "platform/windows/windowsWindow.h"

#include "SirEngine/events/applicationEvent.h"
#include "SirEngine/events/keyboardEvent.h"
#include "SirEngine/events/mouseEvent.h"
#include "SirEngine/log.h"

#include <windowsx.h>

#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/swapChain.h"

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
      KeyTypeEvent e{static_cast<unsigned int>(wparam)};
      ASSERT_CALLBACK_AND_DISPATCH(e);
    }
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
    float movementY = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam));
    // side tilt of the scroll currently not supported, always 0.0f
    MouseScrollEvent e{0.0f, movementY};
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
  dx12::DX12Handles::swapChain->resize(&dx12::DX12Handles::currenFrameResource->fc,
                                       m_data.width, m_data.height);

  dx12::D3DBuffer *m_fontTextureDescriptor = nullptr;
  int m_descriptorIndex;
  assert(m_fontTextureDescriptor == nullptr);
  m_fontTextureDescriptor = new dx12::D3DBuffer();
  m_descriptorIndex = dx12::DX12Handles::globalCBVSRVUAVheap->reserveDescriptor(
      m_fontTextureDescriptor);
}

void WindowsWindow::render() {}

void WindowsWindow::onUpdate() {

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

void WindowsWindow::onResize(unsigned int width, unsigned int height) {
  m_data.width = width;
  m_data.height = height;
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
