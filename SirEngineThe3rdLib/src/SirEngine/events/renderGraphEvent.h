#pragma once

#include "SirEngine/events/event.h"

namespace SirEngine {
class  RenderGraphChanged final : public Event {
public:
  RenderGraphChanged() = default;

  EVENT_CLASS_TYPE(RenderGraphChanged)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryRendering)
};

} // namespace SirEngine
