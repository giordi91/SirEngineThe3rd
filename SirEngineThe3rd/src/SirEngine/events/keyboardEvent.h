#pragma once

#include "SirEngine/events/event.h"

namespace SirEngine {
class SIR_ENGINE_API KeyboardPressEvent : public Event {
public:
  KeyboardPressEvent(unsigned int button) : m_button(button) {}

  EVENT_CLASS_TYPE(WindowResize)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryKeyboard) std::string
      toString() const override {
    std::stringstream s;
    s << "KeyboardPressEvent: " << m_button;
    return s.str();
  }

private:
  unsigned int m_button;
};

class SIR_ENGINE_API KeyboardReleaseEvent : public Event {
public:
  KeyboardReleaseEvent(unsigned int button) : m_button(button) {}

  EVENT_CLASS_TYPE(WindowResize)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryKeyboard) std::string
      toString() const override {
    std::stringstream s;
    s << "KeyboardReleaseEvent: " << m_button;
    return s.str();
  }

private:
  unsigned int m_button;
};
} // namespace SirEngine
