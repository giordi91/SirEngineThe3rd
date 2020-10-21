#pragma once

#include "SirEngine/events/event.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/runtimeString.h"

namespace SirEngine {
class  DebugLayerChanged final : public Event {
public:
  explicit DebugLayerChanged(const int newLayerToShow)
      : m_newLayerToShow(newLayerToShow) {}

  EVENT_CLASS_TYPE(DebugLayerChanged)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryDebug);
  [[nodiscard]] const char *toString() const override {
    char tempLayer[30];
    _itoa(m_newLayerToShow, tempLayer, 10);
    return frameConcatenation("DebugLayer changed: ", tempLayer);
  }
  inline int getLayer() const { return m_newLayerToShow; }

private:
  int m_newLayerToShow = 0;
};

class  DebugRenderConfigChanged final : public Event {
public:
  explicit DebugRenderConfigChanged(DebugLayerConfig config)
      : m_config{config} {}

  EVENT_CLASS_TYPE(DebugRenderChanged)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryDebug);
  inline DebugLayerConfig getConfig() const { return m_config; }

private:
  const DebugLayerConfig m_config;
};

} // namespace SirEngine
