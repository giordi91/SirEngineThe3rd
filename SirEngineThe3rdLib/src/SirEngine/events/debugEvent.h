#pragma once

#include "SirEngine/events/event.h"
#include <sstream>

namespace SirEngine {
class SIR_ENGINE_API DebugLayerChanged : public Event {
public:
  DebugLayerChanged(int newLayerToShow) : m_newLayerToShow(newLayerToShow) {}

  EVENT_CLASS_TYPE(DebugLayerChanged)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryDebug);
  std::string toString() const override {
    std::stringstream s;
    s << "DebugLayer changed: " << m_newLayerToShow;
    return s.str();
  }
  inline int getLayer() const { return m_newLayerToShow; }

private:
  int m_newLayerToShow = 0;
};

class SIR_ENGINE_API DebugDepthChanged : public Event {
public:
  DebugDepthChanged(const float minValue, const float maxValue)
      : m_minValue(minValue), m_maxValue(maxValue) {}

  EVENT_CLASS_TYPE(DebugDepthChanged)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryDebug);
  std::string toString() const override {
    std::stringstream s;
    s << "DebugDepth changed: " << m_minValue << " " << m_maxValue;
    return s.str();
  }
  inline int getMin() const { return m_minValue; }
  inline int getMax() const { return m_maxValue; }

private:
  const float m_minValue;
  const float m_maxValue;
};

} // namespace SirEngine
