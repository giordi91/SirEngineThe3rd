#pragma once

namespace SirEngine {
class Graph;
namespace debug {
struct GraphStatus;
struct RenderGraphWidget final {
  RenderGraphWidget() = default;
  ~RenderGraphWidget();
  void initialize(Graph *graph);
  void render();
  void showGraph(bool value);

  // this is a pointer mostly so we can have the forward declaration
  // without need to leak the graph include and all the imgui stuff
  GraphStatus *status = nullptr;
  bool debugRendering = true;
  int currentDebugLayer = 0;


	//debug specific data:
  float minDepth = 1.0f;
  float maxDepth = 0.0f;

};

} // namespace debug
} // namespace SirEngine
