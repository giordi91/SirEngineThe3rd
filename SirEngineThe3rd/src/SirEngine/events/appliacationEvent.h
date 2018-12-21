#pragma once

#include "SirEngine/events/event.h"

namespace SirEngine {

class SIR_ENGINE_API WindowCloseEvent : public Event {
public:
  WindowCloseEvent() = default;

  EVENT_CLASS_TYPE(WindowClose)
  EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class SIR_ENGINE_API WindowResizeEvent : public Event {
public:
  WindowResizeEvent(unsigned int width, unsigned int height)
      : m_width(width), m_height(height) {}

  EVENT_CLASS_TYPE(WindowResize)
  EVENT_CLASS_CATEGORY(EventCategoryApplication)
  std::string toString() const override {
    std::stringstream s;
    s << "WindowResizeEvent: " << m_width << "x" << m_height;
    return s.str();
  }

private:
  unsigned int m_width;
  unsigned int m_height;
};

} // namespace SirEngine
