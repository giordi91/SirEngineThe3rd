#pragma once

#pragma once
#include "SirEngine/core.h"
#include "SirEngine/memory/resizableVector.h"

namespace SirEngine {

class Layer;
template class SIR_ENGINE_API ResizableVector<Layer*>;
class SIR_ENGINE_API LayerStack final {
public:
  LayerStack() : m_layers(20) {}
  ~LayerStack();

  void pushLayer(Layer *layer);
  void popLayer(Layer *layer);
  inline Layer **begin() const { return m_layers.data(); };
  inline uint32_t count() const { return m_layers.size(); };

private:
  ResizableVector<Layer *> m_layers;
};
} // namespace SirEngine
