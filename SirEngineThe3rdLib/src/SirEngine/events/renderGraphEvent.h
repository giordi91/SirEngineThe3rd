#pragma once

#include "SirEngine/events/event.h"

namespace SirEngine {
class SIR_ENGINE_API RenderGraphChanged final : public Event {
public:
  RenderGraphChanged() = default;

  EVENT_CLASS_TYPE(RenderGraphChanged)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryRendering)
  std::string toString() const override { return "RenderGraph changed"; }
};

} // namespace SirEngine
