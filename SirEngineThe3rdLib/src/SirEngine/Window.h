#pragma once
#include "SirEngine/events/event.h"
#include "core.h"

namespace SirEngine {
struct WindowProps {
  const char *title = "Sir Engine the 3rd";
  uint32_t width = 1280;
  uint32_t height = 720;
  bool vsync = false;
  bool startFullScreen = true;
};

struct NativeWindow {
  uint64_t data;
  uint64_t data2;
};

class SIR_ENGINE_API BaseWindow {
public:
  using EventCallbackFn = std::function<void(Event &)>;

  virtual ~BaseWindow() = default;
  virtual void onUpdate() = 0;
  virtual void onResize(uint32_t width, uint32_t height) = 0;

  virtual uint32_t getWidth() const = 0;
  virtual uint32_t getHeight() const = 0;

  // window attributes
  virtual void setEventCallback(const EventCallbackFn &callback) = 0;
  virtual void setVSync(bool ennabled) = 0;
  virtual void isVSync() const = 0;
  virtual const NativeWindow *getNativeWindow() const = 0;

  // This needs to be implemented per platform
  static BaseWindow *create(const WindowProps &props = WindowProps());

protected:
protected:
  NativeWindow m_nativeWindow = {};
};

} // namespace SirEngine
