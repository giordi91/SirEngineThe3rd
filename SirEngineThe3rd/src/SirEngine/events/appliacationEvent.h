#pragma once

#include "SirEngine/events/event.h"

namespace SirEngine {

class SIR_ENGINE_API WindowCloseEvent : public Event {
public:
  WindowCloseEvent() = default;

  EVENT_CLASS_TYPE(WindowClose)
  EVENT_CLASS_CATEGORY(EventCategoryApplication)
};
} // namespace SirEngine
