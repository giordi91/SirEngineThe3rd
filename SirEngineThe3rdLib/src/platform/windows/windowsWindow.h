#pragma once
#include "SirEngine/Window.h"
#include <Windows.h>

namespace SirEngine {
namespace dx12 {
class SwapChain;
class D3DBuffer;
} // namespace dx12
class WindowsWindow : public Window {

public:
  explicit WindowsWindow(const WindowProps &props);
  virtual ~WindowsWindow() = default;
  void OnUpdate() override;

  unsigned int getWidth() const override;
  unsigned int getHeight() const override;

  // window attributes
  virtual void setEventCallback(const EventCallbackFn &callback) override;
  void setVSync(bool ennabled) override;
  void isVSync() const override;
  inline EventCallbackFn getEventCallback() const { return m_callback; }

private:
  struct WindowData {
    std::string title;
    unsigned int width;
    unsigned int height;
  };
  void render();

private:
  HINSTANCE m_hinstance;
  HWND m_hwnd;
  WindowData m_data;
  EventCallbackFn m_callback;
};

} // namespace SirEngine
