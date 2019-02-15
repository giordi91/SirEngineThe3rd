#pragma once
#include "SirEngine/core.h"
#include <string>
#include "SirEngine/events/event.h"

namespace SirEngine {

class SIR_ENGINE_API Layer {

public:
  using EventCallbackFn = std::function<void(Event &)>;

  Layer() : m_debugName("Layer") {}
  Layer(const char *debugName) : m_debugName(debugName) {}
  virtual ~Layer() = default;

  virtual void onAttach() = 0;
  virtual void onDetach() = 0;
  virtual void onUpdate() = 0;
  virtual void onEvent(Event &event) = 0;
  virtual void clear() = 0;


  inline const std::string getName() { return m_debugName; }

protected:
  std::string m_debugName;
};

} // namespace SirEngine
