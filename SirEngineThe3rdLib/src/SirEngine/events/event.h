#pragma once
#include <functional>

#include "SirEngine/core.h"

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
  RenderGraphChanged,
  ShaderCompile,
  RequestShaderCompile,
  ShaderCompileResult,
  ReloadScripts,
  RenderSizeChanged
};

enum EventCategory {
  NONE = 0,
  EventCategoryApplication = SET_BIT(0),
  EventCategoryInput = SET_BIT(1),
  EventCategoryKeyboard = SET_BIT(2),
  EventCategoryMouse = SET_BIT(3),
  EventCategoryMouseButton = SET_BIT(4),
  EventCategoryDebug = SET_BIT(5),
  EventCategoryRendering = SET_BIT(6),
  EventCategoryShaderCompile = SET_BIT(7)
};

#define EVENT_CLASS_TYPE(type)                                                \
  static EventType getStaticType() { return EventType::##type; }              \
  virtual EventType getEventType() const override { return getStaticType(); } \
  virtual const char *getName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) \
  virtual int getCategoryFlags() const override { return category; }

class Event {
  friend class EventDispatcher;

 public:
  virtual ~Event() = default;
  virtual EventType getEventType() const = 0;
  virtual const char *getName() const = 0;
  virtual int getCategoryFlags() const = 0;
  virtual const char *toString() const { return getName(); }

  inline bool isInCategory(const EventCategory category) const {
    return getCategoryFlags() & category;
  }
  inline bool handled() const { return m_handled; };
  inline void setHandled(const bool handled) { m_handled = handled; }

 protected:
  bool m_handled = false;
};

// TODO refactor this
// I really really REALLY don't like this, and should go
class EventDispatcher {
  template <typename T>
  using EventFn = std::function<bool(T &)>;

 public:
  explicit EventDispatcher(Event &event) : m_event(event) {}

  // This is the dispatcher, the way the dispatcher works is the following,
  // the dispatcher has been created with an event, which is going to reference
  // to. then when dispatch is called, if the type matches, we are going to call
  // the function on the event the dispatcher was associated to. The function
  // will be
  template <typename T>
  bool dispatch(EventFn<T> funct) {
    if (m_event.getEventType() == T::getStaticType()) {
      m_event.m_handled = funct(*(T *)&m_event);
      return true;
    }
    return false;
  };

 private:
  Event &m_event;
};

}  // namespace SirEngine
