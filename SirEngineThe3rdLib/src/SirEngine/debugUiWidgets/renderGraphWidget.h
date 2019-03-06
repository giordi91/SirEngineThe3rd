#pragma once
#include "SirEngine/graphics/cpuGraphicsStructures.h"

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
  Graph* m_graph;
  bool debugRendering = true;
  int currentDebugLayer = 0;

  // debug specific data:
  DebugLayerConfig m_debugConfig;
  float depthMinStart = 1.0f;
  float depthMinEnd= 0.0f;
  float depthMaxStart = 1.0f;
  float depthMaxEnd=0.0f;
};

} // namespace debug
} // namespace SirEngine
