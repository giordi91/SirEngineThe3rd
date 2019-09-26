#include "SirEngine/layerStack.h"
#include "SirEngine/layer.h"

namespace SirEngine {
LayerStack::~LayerStack() {
  /*
for (Layer *l : m_layers) {
delete l;
}
*/
}
void LayerStack::pushLayer(Layer *layer) {
  m_layers.pushBack(layer);
  layer->onAttach();
}
void LayerStack::popLayer(Layer *) {
  assert(0 && "layer pop is not implemnted yet");
}
} // namespace SirEngine
