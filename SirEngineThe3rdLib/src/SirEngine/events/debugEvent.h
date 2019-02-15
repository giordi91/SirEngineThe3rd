#pragma once

#include "SirEngine/events/event.h"
#include <sstream> 

namespace SirEngine {
class SIR_ENGINE_API DebugLayerChanged: public Event {
public:
  DebugLayerChanged(int newLayerToShow ) : m_newLayerToShow(newLayerToShow) {}

  EVENT_CLASS_TYPE(DebugLayerChanged)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryDebug);
  std::string toString() const override {
    std::stringstream s;
    s << "DebugLayer changed: " <<m_newLayerToShow;
    return s.str();
  }
  inline int getLayer() const { return m_newLayerToShow; }

private:
	int m_newLayerToShow =0;
};

} // namespace SirEngine
