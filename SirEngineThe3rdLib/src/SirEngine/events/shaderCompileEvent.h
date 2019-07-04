#pragma once

#include "SirEngine/events/event.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include <sstream>

namespace SirEngine {
class SIR_ENGINE_API ShaderCompileEvent : public Event {
public:
  ShaderCompileEvent(const char *shaderToCompile)
      : m_shaderToCompile(shaderToCompile) {}

  EVENT_CLASS_TYPE(ShaderCompile)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryShaderCompile);
  std::string toString() const override {
    std::stringstream s;
    s << "Requested shader compilation: " << m_shaderToCompile;
    return s.str();
  }
  inline const char *getShader() const { return m_shaderToCompile.c_str(); }

private:
  // this is a bit heavy but is for debug messages, for the time being will do
  const std::string m_shaderToCompile;
};

class SIR_ENGINE_API ShaderCompileResultEvent : public Event {
public:
  ShaderCompileResultEvent(const char *log) : m_log{log} {}

  EVENT_CLASS_TYPE(ShaderCompileResult)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryShaderCompile);
  std::string toString() const override {
    std::stringstream s;
    s << "compilation log: "<<m_log;
    return s.str();
  }
  inline const char *getLog() const { return m_log.c_str(); }

private:
  const std::string m_log;
};

} // namespace SirEngine
