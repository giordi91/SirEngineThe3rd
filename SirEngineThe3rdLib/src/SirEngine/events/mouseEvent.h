#pragma once

#include "SirEngine/events/event.h"

namespace SirEngine {
class SIR_ENGINE_API MouseMoveEvent final : public Event {
public:
  MouseMoveEvent(const float posX, const float posY)
      : m_posX(posX), m_posY(posY) {}

  EVENT_CLASS_TYPE(MouseMoved)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryMouse)
  [[nodiscard]] const char *toString() const override {
    const char *number = frameConcatenation(m_posX, m_posY, ",");
    return frameConcatenation("MouseMoveEvent: ", number);
  }
  inline float getX() const { return m_posX; }
  inline float getY() const { return m_posY; }

private:
  float m_posX;
  float m_posY;
};

class SIR_ENGINE_API MouseScrollEvent final : public Event {
public:
  MouseScrollEvent(const float offsetX, const float offsetY)
      : m_offsetX(offsetX), m_offsetY(offsetY) {}

  EVENT_CLASS_TYPE(MouseScrolled)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryMouse)
  [[nodiscard]] const char *toString() const override {
    const char *number = frameConcatenation(m_offsetX, m_offsetY, ",");
    return frameConcatenation("MouseScrollEvent: ", number);
  }

  inline float getOffsetX() const { return m_offsetX; }
  inline float getOffsetY() const { return m_offsetY; }

private:
  float m_offsetX;
  float m_offsetY;
};

enum class MOUSE_BUTTONS_EVENT { LEFT = 0, RIGHT, MIDDLE };

class SIR_ENGINE_API MouseButtonPressEvent final : public Event {
public:
  explicit MouseButtonPressEvent(const MOUSE_BUTTONS_EVENT mouseButton)
      : m_button(mouseButton) {}

  EVENT_CLASS_TYPE(MouseButtonPressed)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryMouse)
  [[nodiscard]] const char *toString() const override {
    const char *first = "MouseButtonPressEvent: ";
    switch (m_button) {
    case (MOUSE_BUTTONS_EVENT::LEFT): {
      return frameConcatenation(first, "left");
    }
    case (MOUSE_BUTTONS_EVENT::RIGHT): {
      return frameConcatenation(first, "right");
    }
    case (MOUSE_BUTTONS_EVENT::MIDDLE): {
      return frameConcatenation(first, "middle");
    }
    default:
      return "";
    }
  }
  inline MOUSE_BUTTONS_EVENT getMouseButton() const { return m_button; };

private:
  MOUSE_BUTTONS_EVENT m_button;
};

class SIR_ENGINE_API MouseButtonReleaseEvent final : public Event {
public:
  explicit MouseButtonReleaseEvent(const MOUSE_BUTTONS_EVENT mouseButton)
      : m_button(mouseButton) {}

  EVENT_CLASS_TYPE(MouseButtonReleased)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryMouse)
  [[nodiscard]] const char *toString() const override {
    const char *first = "MouseButtonReleaseEvent: ";
    switch (m_button) {
    case (MOUSE_BUTTONS_EVENT::LEFT): {
      return frameConcatenation(first, "left");
    }
    case (MOUSE_BUTTONS_EVENT::RIGHT): {
      return frameConcatenation(first, "right");
    }
    case (MOUSE_BUTTONS_EVENT::MIDDLE): {
      return frameConcatenation(first, "middle");
    }
    default:
      return "";
    }
  }
  inline MOUSE_BUTTONS_EVENT getMouseButton() const { return m_button; };

private:
  MOUSE_BUTTONS_EVENT m_button;
};
} // namespace SirEngine
