#pragma once

#include "SirEngine/events/event.h"

namespace SirEngine {
class  KeyboardPressEvent final : public Event {
public:
  KeyboardPressEvent(const uint32_t button) : m_button(button) {}

  EVENT_CLASS_TYPE(KeyPressed)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryKeyboard)
  [[nodiscard]] const char *toString() const override {
    return frameConcatenation("KeyboardPressEvent: ",
                              static_cast<int>(m_button));
  }
  uint32_t getKeyCode() const { return m_button; }

private:
  uint32_t m_button;
};

class  KeyboardReleaseEvent : public Event {
public:
  explicit KeyboardReleaseEvent(const uint32_t button) : m_button(button) {}

  EVENT_CLASS_TYPE(KeyReleased)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryKeyboard)
  [[nodiscard]] const char *toString() const override {
    return frameConcatenation("KeyboardPressEvent: ",
                              static_cast<int>(m_button));
  }

  uint32_t getKeyCode() const { return m_button; }

private:
  uint32_t m_button;
};

class  KeyTypeEvent : public Event {
public:
  explicit KeyTypeEvent(const uint32_t button) : m_button(button) {}

  EVENT_CLASS_TYPE(KeyTyped)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryKeyboard)
  [[nodiscard]] const char *toString() const override {
    return frameConcatenation("Keyboard typed char: ",
                              static_cast<int>(m_button));
  }
  inline uint32_t getKeyCode() const { return m_button; }

private:
  uint32_t m_button;
};

} // namespace SirEngine
