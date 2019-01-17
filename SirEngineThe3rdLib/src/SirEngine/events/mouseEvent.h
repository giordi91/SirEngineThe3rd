#pragma once

#include "SirEngine/events/event.h"

namespace SirEngine {
class SIR_ENGINE_API MouseMoveEvent : public Event {
public:
  MouseMoveEvent(float posX, float posY) : m_posX(posX), m_posY(posY) {}

  EVENT_CLASS_TYPE(MouseMoved)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryMouse)
  std::string toString() const override {
    std::stringstream s;
    s << "MouseMoveEvent: (" << m_posX << "," << m_posY << ")";
    return s.str();
  }
  inline float getX() { return m_posX; }
  inline float getY() { return m_posY; }

private:
  float m_posX;
  float m_posY;
};

class SIR_ENGINE_API MouseScrollEvent : public Event {
public:
  MouseScrollEvent(float offsetX, float offsetY)
      : m_offsetX(offsetX), m_offsetY(offsetY) {}

  EVENT_CLASS_TYPE(MouseScrolled)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryMouse)
  std::string toString() const override {
    std::stringstream s;
    s << "MouseScrollEvent: (" << m_offsetX << "," << m_offsetY << ")";
    return s.str();
  }

  inline float getOffsetX() { return m_offsetX; }
  inline float getOffsetY() { return m_offsetY; }

private:
  float m_offsetX;
  float m_offsetY;
};

enum class MOUSE_BUTTONS_EVENT { LEFT = 0, RIGHT, MIDDLE };

class SIR_ENGINE_API MouseButtonPressEvent : public Event {
public:
  MouseButtonPressEvent(MOUSE_BUTTONS_EVENT mouseButton)
      : m_button(mouseButton) {}

  EVENT_CLASS_TYPE(MouseButtonPressed)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryMouse)
  std::string toString() const override {
    std::stringstream s;
    s << "MouseButtonPressEvent: ";
    switch (m_button) {
    case (MOUSE_BUTTONS_EVENT::LEFT): {
      s << "left";
      break;
    }
    case (MOUSE_BUTTONS_EVENT::RIGHT): {
      s << "right";
      break;
    }
    case (MOUSE_BUTTONS_EVENT::MIDDLE): {
      s << "middle";
      break;
    }
    }
    return s.str();
  }
  inline MOUSE_BUTTONS_EVENT getMouseButton() { return m_button; };

private:
  MOUSE_BUTTONS_EVENT m_button;
};

class SIR_ENGINE_API MouseButtonReleaseEvent : public Event {
public:
  MouseButtonReleaseEvent(MOUSE_BUTTONS_EVENT mouseButton)
      : m_button(mouseButton) {}

  EVENT_CLASS_TYPE(MouseButtonReleased)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput |
                       EventCategory::EventCategoryMouse)
  std::string toString() const override {
    std::stringstream s;
    s << "MouseButtonReleaseEvent: ";
    switch (m_button) {
    case (MOUSE_BUTTONS_EVENT::LEFT): {
      s << "left";
      break;
    }
    case (MOUSE_BUTTONS_EVENT::RIGHT): {
      s << "right";
      break;
    }
    case (MOUSE_BUTTONS_EVENT::MIDDLE): {
      s << "middle";
      break;
    }
    }
    return s.str();
  }
  inline MOUSE_BUTTONS_EVENT getMouseButton() { return m_button; };

private:
  MOUSE_BUTTONS_EVENT m_button;
};
} // namespace SirEngine
