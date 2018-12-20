#pragma once
#include "SirEnginepch.h"
#include "SirEngine/events/event.h"
#include "core.h"

namespace SirEngine {
struct WindowProps {
  std::string title = "Sir Engine Window";
  unsigned int width = 1280;
  unsigned int height = 720;
};

class SIR_ENGINE_API Window {
public:
  using EventCallbackFn = std::function<void(Event&)>;

  virtual ~Window() = default;
  virtual void OnUpdate() = 0;

  virtual unsigned int getWidth() const = 0;
  virtual unsigned int getHeight() const = 0;

  // window attributes
  virtual void setEventCallback(const EventCallbackFn& callback) = 0;
  virtual void setVSync(bool ennabled) = 0;
  virtual void isVSync() const = 0;

  // This needs to be implemented per platform
  static Window *create(const WindowProps &props = WindowProps());
};
} // namespace SirEngine
