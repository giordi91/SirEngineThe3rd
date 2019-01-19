#include "SirEngine/layers/graphics3DLayer.h"
#include "SirEngine/globals.h"

namespace SirEngine {
void Graphics3DLayer::onAttach() {
	Globals::mainCamera = new Camera3dPivot();

}
void Graphics3DLayer::onDetach() {}
void Graphics3DLayer::onUpdate() {



}
void Graphics3DLayer::onEvent(Event &event) {}
} // namespace SirEngine
