#pragma once

#include "SirEngine/events/event.h"

namespace SirEngine {
class RenderGraphChanged final : public Event {
 public:
  RenderGraphChanged() = default;

  EVENT_CLASS_TYPE(RenderGraphChanged)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryRendering)
};

class RenderSizeChanged final : public Event {
 public:
  RenderSizeChanged(const uint32_t width, const uint32_t height)
      : m_width(width), m_height(height) {}

  EVENT_CLASS_TYPE(RenderSizeChanged)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryRendering)
  [[nodiscard]] unsigned int getWidth() const {
    return m_width;
  }
  [[nodiscard]] unsigned int getHeight() const { return m_height; }

  [[nodiscard]] const char* toString() const override {
    char widthStr[30];
    char heightStr[30];
    _itoa(m_width, widthStr, 10);
    _itoa(m_height, heightStr, 10);
    const char* temp = frameConcatenation(widthStr, heightStr, "x");
    return frameConcatenation("RenderSizeChanged: ", temp);
  }

 private:
  unsigned int m_width;
  unsigned int m_height;
};

}  // namespace SirEngine
