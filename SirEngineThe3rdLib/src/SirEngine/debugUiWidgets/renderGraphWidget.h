#pragma once
#include "SirEngine/graphics/cpuGraphicsStructures.h"

namespace SirEngine {
class DependencyGraph;
namespace debug {
struct GraphStatus;
struct RenderGraphWidget final {
  RenderGraphWidget() = default;
  ~RenderGraphWidget();
  void initialize(DependencyGraph*graph);
  void render();
  void showGraph(bool value);

  // this is a pointer mostly so we can have the forward declaration
  // without need to leak the graph include and all the imgui stuff
  GraphStatus *status = nullptr;
  DependencyGraph* m_graph;
  bool debugRendering = true;
  int currentDebugLayer = 0;

  // debug specific data:
  DebugLayerConfig m_debugConfig;
};

} // namespace debug
} // namespace SirEngine
