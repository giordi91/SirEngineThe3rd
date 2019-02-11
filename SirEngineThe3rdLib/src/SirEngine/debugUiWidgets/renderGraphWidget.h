#pragma once

namespace SirEngine {
class Graph;

namespace debug {
class GraphStatus;
struct RenderGraphWidget final {
  RenderGraphWidget() = default;
  ~RenderGraphWidget();
  void initialize(Graph *graph);
  void render();

  // this is a pointer mostly so we can have the forward declaration
  // without need to leak the graph include and all the imgui stuff
  GraphStatus *status = nullptr;
};

}; // namespace debug
} // namespace SirEngine
