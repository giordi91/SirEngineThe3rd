#pragma once
#include <cstdint>

namespace SirEngine {
class Graph;

namespace debug {
struct RenderGraphWidget final {
	void initialize(Graph* graph);
  void render();
};

}; // namespace debug
} // namespace SirEngine
