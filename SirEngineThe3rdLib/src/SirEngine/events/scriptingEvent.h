#pragma once

#include "SirEngine/events/event.h"

namespace SirEngine {

class SIR_ENGINE_API ReloadScriptsEvent final : public Event {
public:
  ReloadScriptsEvent() = default;

  EVENT_CLASS_TYPE(ReloadScripts)
  EVENT_CLASS_CATEGORY(EventCategoryDebug)
};
} // namespace SirEngine
