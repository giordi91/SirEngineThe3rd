#pragma once

#include "SirEngine/events/event.h"
#include "SirEngine/runtimeString.h"

namespace SirEngine {
class SIR_ENGINE_API ShaderCompileEvent final : public Event {
public:
  ShaderCompileEvent(const char *shaderToCompile, const char *offsetPath)
      : m_shaderToCompile(persistentString(shaderToCompile)),
        m_offsetPath(persistentString(offsetPath)) {}

  virtual ~ShaderCompileEvent() {
    stringFree(m_shaderToCompile);
    stringFree(m_offsetPath);
  }

  EVENT_CLASS_TYPE(ShaderCompile)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryShaderCompile);

  [[nodiscard]] const char *toString() const override {
    const char *nameTemp =
        frameConcatenation("Requested shader compilation: ", m_shaderToCompile);
    const char *offsetPathTemp =
        frameConcatenation("\n Offset path: ", m_offsetPath);
    return frameConcatenation(nameTemp, offsetPathTemp);
  }
  inline const char *getShader() const { return m_shaderToCompile; }
  inline const char *getOffsetPath() const { return m_offsetPath; }

private:
  // this is a bit heavy but is for debug messages, for the time being will do
  const char *m_shaderToCompile;
  const char *m_offsetPath;
};
class SIR_ENGINE_API RequestShaderCompileEvent final : public Event {
public:
  RequestShaderCompileEvent() = default;

  EVENT_CLASS_TYPE(RequestShaderCompile)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryShaderCompile);

  [[nodiscard]] const char *toString() const override {
    return "Requested shader compilation event \n";
  }
};

class SIR_ENGINE_API ShaderCompileResultEvent final : public Event {
public:
  explicit ShaderCompileResultEvent(const char *log)
      : m_log(persistentString(log)) {}

  EVENT_CLASS_TYPE(ShaderCompileResult)
  EVENT_CLASS_CATEGORY(EventCategory::EventCategoryShaderCompile);
  [[nodiscard]] const char *toString() const override {
    return frameConcatenation("Compilation log: ", m_log);
  }
  inline const char *getLog() const { return m_log; }

private:
  const char *m_log;
};

} // namespace SirEngine
