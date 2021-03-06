#pragma once

#include "SirEngine/events/event.h"
#include "SirEngine/runtimeString.h"

namespace SirEngine {

class WindowCloseEvent final : public Event {
 public:
  WindowCloseEvent() = default;

  EVENT_CLASS_TYPE(WindowClose)
  EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class WindowResizeEvent final : public Event {
 public:
  WindowResizeEvent(const uint32_t width, const uint32_t height)
      : m_width(width), m_height(height) {}

  EVENT_CLASS_TYPE(WindowResize)
  EVENT_CLASS_CATEGORY(EventCategoryApplication)

  [[nodiscard]] const char* toString() const override {
    char widthStr[30];
    char heightStr[30];
    _itoa(m_width, widthStr, 10);
    _itoa(m_height, heightStr, 10);
    const char* temp = frameConcatenation(widthStr, heightStr, "x");
    return frameConcatenation("WindowResizeEvent: ", temp);
  }

  [[nodiscard]] unsigned int getWidth() const { return m_width; }
  [[nodiscard]] unsigned int getHeight() const { return m_height; }

 private:
  unsigned int m_width;
  unsigned int m_height;
};

}  // namespace SirEngine
