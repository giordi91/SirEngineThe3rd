#pragma once

#include "SirEngine/events/event.h"
#include <sstream>

namespace SirEngine {
class SIR_ENGINE_API KeyboardPressEvent : public Event {
public:
  KeyboardPressEvent(unsigned int button) : m_button(button) {}

  EVENT_CLASS_TYPE(KeyPressed)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryKeyboard) std::string
      toString() const override {
    std::stringstream s;
    s << "KeyboardPressEvent: " << m_button;
    return s.str();
  }
  unsigned int getKeyCode() const { return m_button; }

private:
  unsigned int m_button;
};

class SIR_ENGINE_API KeyboardReleaseEvent : public Event {
public:
  KeyboardReleaseEvent(unsigned int button) : m_button(button) {}

  EVENT_CLASS_TYPE(KeyReleased)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryKeyboard) std::string
      toString() const override {
    std::stringstream s;
    s << "KeyboardReleaseEvent: " << m_button;
    return s.str();
}

  unsigned int getKeyCode() const { return m_button; }

private:
  unsigned int m_button;
};


class SIR_ENGINE_API KeyTypeEvent: public Event {
public:
  KeyTypeEvent(unsigned int button) : m_button(button) {}

  EVENT_CLASS_TYPE(KeyTyped)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryKeyboard) std::string
      toString() const override {
    std::stringstream s;
    s << "Keyboard typed char: " << m_button;
    return s.str();
  }
  inline unsigned int getKeyCode() const { return m_button; }

private:
  unsigned int m_button;
};

} // namespace SirEngine
