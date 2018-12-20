#pragma once
#include "SirEngine/Window.h"

namespace SirEngine {
class WindowsWindow : public Window {

public:
  explicit WindowsWindow(const WindowProps &props);
  virtual ~WindowsWindow() = default;
  void OnUpdate() override;

  unsigned int getWidth() const override;
  unsigned int getHeight() const override;

  // window attributes
  // virtual void setEvantCallback(const EventCallbackFn& callback) = 0;
  void setVSync(bool ennabled) override;
  void isVSync() const override;

  LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);
private:

  struct WindowData {
    std::string title;
    unsigned int width;
    unsigned int height;
  };

private:
  LPCWSTR m_applicationName;
  HINSTANCE m_hinstance;
  HWND m_hwnd;
  WindowData m_data;
};

} // namespace SirEngine

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static SirEngine::WindowsWindow *windowsApplicationHandle = nullptr;
