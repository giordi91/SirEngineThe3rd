#pragma once
#include "SirEngine/core.h"
#include "SirEngine/events/event.h"
#include <string>

namespace SirEngine {

class SIR_ENGINE_API Layer {
public:
  Layer(std::string &debugName) : m_debugName(debugName) {}
  virtual ~Layer() = default;

  virtual void onAttach() = 0;
  virtual void onDetach() = 0;
  virtual void onUpdate() = 0;
  virtual void onEvent() = 0;

  inline const std::string getName() { return m_debugName; }

private:
  std::string m_debugName;
};

} // namespace SirEngine
