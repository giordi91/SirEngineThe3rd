#include "SirEngine/layerStack.h"
#include "SirEngine/layer.h"

namespace SirEngine {
LayerStack::LayerStack() { m_layerInsert = m_layers.begin(); }
LayerStack::~LayerStack() {
  for (Layer *l : m_layers) {
    delete l;
  }
}
void LayerStack::pushLayer(Layer *layer) {
  m_layerInsert = m_layers.emplace(m_layerInsert, layer);
  layer->onAttach();
}
void LayerStack::popLayer(Layer *layer) {

  auto it = std::find(m_layers.begin(), m_layers.end(), layer);
  if (it != m_layers.end()) {
    m_layers.erase(it);
  }
}
void LayerStack::pushOverlayLayer(Layer *layer) {
  m_layers.emplace_back(layer);
  layer->onAttach();
}
void LayerStack::popOverlayLayer(Layer *layer) {
  auto it = std::find(m_layers.begin(), m_layers.end(), layer);
  if (it != m_layers.end()) {
    m_layers.erase(it);
    m_layerInsert--;
  }
}
} // namespace SirEngine
