#pragma once
#include "SirEngine/core.h"
#include <functional>
#include <string>

namespace SirEngine {

enum class EventType {
  NONE = 0,
  WindowClose,
  WindowResize,
  WindowFocus,
  WindowLostFocus,
  WindowMoved,
  KeyPressed,
  KeyReleased,
  KeyTyped,
  MouseButtonPressed,
  MouseButtonReleased,
  MouseMoved,
  MouseScrolled,
  DebugLayerChanged,
  DebugRenderChanged,
  RenderGraphChanged
};

enum EventCategory {
  NONE = 0,
  EventCategoryApplication = SET_BIT(0),
  EventCategoryInput = SET_BIT(1),
  EventCategoryKeyboard = SET_BIT(2),
  EventCategoryMouse = SET_BIT(3),
  EventCategoryMouseButton = SET_BIT(4),
  EventCategoryDebug = SET_BIT(5),
  EventCategoryRendering = SET_BIT(6)
};

#define EVENT_CLASS_TYPE(type)                                                 \
  static EventType getStaticType() { return EventType::##type; }               \
  virtual EventType getEventType() const override { return getStaticType(); }  \
  virtual const char *getName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category)                                         \
  virtual int getCategoryFlags() const override { return category; }

class SIR_ENGINE_API Event {
  friend class EventDispatcher;

public:
  virtual ~Event() = default;
  virtual EventType getEventType() const = 0;
  virtual const char *getName() const = 0;
  virtual int getCategoryFlags() const = 0;
  virtual std::string toString() const { return getName(); }

  inline bool isInCategory(EventCategory category) {
    return getCategoryFlags() & category;
  }
  inline bool handled() { return m_handled; };
  inline void setHandled(bool handled) { m_handled = handled; }

protected:
  bool m_handled = false;
};

class EventDispatcher {
  template <typename T> using EventFn = std::function<bool(T &)>;

public:
  EventDispatcher(Event &event) : m_event(event) {}

  // This is the dispatcher, the way the dispatcher works is the following,
  // the dispatcher has been created with an event, which is going to reference
  // to. then when dispatch is called, if the type matches, we are going to call
  // the function on the event the dispatcher was associated to. The function
  // will be
  template <typename T> bool dispatch(EventFn<T> funct) {
    if (m_event.getEventType() == T::getStaticType()) {
      m_event.m_handled = funct(*(T *)&m_event);
      return true;
    }
    return false;
  };

private:
  Event &m_event;
};
inline std::ostream &operator<<(std::ostream &os, const Event &e) {
  return os << e.toString();
}

} // namespace SirEngine
