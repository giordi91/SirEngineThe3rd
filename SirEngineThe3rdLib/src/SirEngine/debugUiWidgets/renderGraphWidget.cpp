#include "SirEngine/debugUiWidgets/renderGraphWidget.h"
#include "SirEngine/graphics/nodeGraph.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "SirEngine/log.h"
#include <queue>
#include <unordered_set>

#include "SirEngine/application.h"
#include "SirEngine/events/debugEvent.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/postProcess/effects/gammaAndToneMappingEffect.h"
#include "SirEngine/graphics/postProcess/postProcessStack.h"

namespace SirEngine {
namespace debug {

enum class PostProcessTypeDebug { NONE, GAMMA_TONE_MAPPING };

const std::unordered_map<std::string, PostProcessTypeDebug>
    POST_PROCESS_TYPE_TO_ENUM{{"GammaAndToneMappingEffect",
                               PostProcessTypeDebug::GAMMA_TONE_MAPPING}};

PostProcessTypeDebug getPostProcessDebugType(const std::string &name) {
  auto found = POST_PROCESS_TYPE_TO_ENUM.find(name);
  if (found != POST_PROCESS_TYPE_TO_ENUM.end()) {
    return found->second;
  }
  return PostProcessTypeDebug::NONE;
}

struct Node {
  int ID;
  char Name[64];
  ImVec2 Pos, Size;
  int InputsCount, OutputsCount;
  const GraphNode *node;

  Node(int id, const char *name, const ImVec2 &pos, int inputs_count,
       int outputs_count, const GraphNode *inNode) {
    ID = id;
    strncpy(Name, name, 31);
    Name[31] = 0;
    Pos = pos;
    InputsCount = inputs_count;
    OutputsCount = outputs_count;
    node = inNode;
  }

  ImVec2 GetInputSlotPos(int slot_no) const {
    return ImVec2(Pos.x, Pos.y + Size.y * ((float)slot_no + 1) /
                                     ((float)InputsCount + 1));
  }
  ImVec2 GetOutputSlotPos(int slot_no) const {
    return ImVec2(Pos.x + Size.x, Pos.y + Size.y * ((float)slot_no + 1) /
                                              ((float)OutputsCount + 1));
  }
};
struct NodeLink {
  int InputIdx, InputSlot, OutputIdx, OutputSlot;

  NodeLink(int input_idx, int input_slot, int output_idx, int output_slot) {
    InputIdx = input_idx;
    InputSlot = input_slot;
    OutputIdx = output_idx;
    OutputSlot = output_slot;
  }
};

struct GraphStatus {
  ImVector<Node> nodes;
  ImVector<NodeLink> links;
  ImVec2 scrolling = ImVec2(0.0f, 0.0f);
  bool show_grid = true;
  int node_selected = -1;
  bool opened = false;
  float GRAPH_WIDTH = 500;
  float GRAPH_HEIGHT = 300;
};

void inline plugToolTip(const char *name) {
  ImGui::SetNextWindowSize(ImVec2(80, 30));
  ImGui::BeginTooltip();
  ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 80);
  ImGui::Text(name);
  ImGui::PopTextWrapPos();
  ImGui::EndTooltip();
}
void renderImguiGraph(GraphStatus *status) {
  ImVec2 winPos{globals::SCREEN_WIDTH - status->GRAPH_WIDTH,
                globals::SCREEN_HEIGHT - status->GRAPH_HEIGHT};
  ImGui::SetNextWindowPos(winPos, ImGuiSetCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(status->GRAPH_WIDTH, status->GRAPH_HEIGHT),
                           ImGuiSetCond_Always);
  if (!status->opened) {
    return;
  }
  if (!ImGui::Begin("Render Graph", &status->opened)) {
    ImGui::End();
    return;
  }

  // Draw a list of nodes on the left side
  bool open_context_menu = false;
  int node_hovered_in_list = -1;
  int node_hovered_in_scene = -1;
  ImGui::BeginChild("node_list", ImVec2(100, 0));
  ImGui::Text("Nodes");
  ImGui::Separator();
  for (int node_idx = 0; node_idx < status->nodes.Size; node_idx++) {
    Node *node = &status->nodes[node_idx];
    ImGui::PushID(node->ID);
    if (ImGui::Selectable(node->Name, node->ID == status->node_selected))
      status->node_selected = node->ID;
    if (ImGui::IsItemHovered()) {
      node_hovered_in_list = node->ID;
      open_context_menu |= ImGui::IsMouseClicked(1);
    }
    ImGui::PopID();
  }
  ImGui::EndChild();

  ImGui::SameLine();
  ImGui::BeginGroup();

  const float NODE_SLOT_RADIUS = 4.0f;
  const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

  // Create our child canvas
  ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)",
              status->scrolling.x, status->scrolling.y);
  ImGui::SameLine(ImGui::GetWindowWidth() - 100);
  ImGui::Checkbox("Show grid", &status->show_grid);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, IM_COL32(60, 60, 70, 200));
  ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
  ImGui::PushItemWidth(120.0f);

  ImVec2 offset = ImGui::GetCursorScreenPos() + status->scrolling;
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  // Display grid
  if (status->show_grid) {
    ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
    float GRID_SZ = 64.0f;
    ImVec2 win_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetWindowSize();
    for (float x = fmodf(status->scrolling.x, GRID_SZ); x < canvas_sz.x;
         x += GRID_SZ)
      draw_list->AddLine(ImVec2(x, 0.0f) + win_pos,
                         ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
    for (float y = fmodf(status->scrolling.y, GRID_SZ); y < canvas_sz.y;
         y += GRID_SZ)
      draw_list->AddLine(ImVec2(0.0f, y) + win_pos,
                         ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
  }

  // Display links
  draw_list->ChannelsSplit(2);
  draw_list->ChannelsSetCurrent(0); // Background
  for (int link_idx = 0; link_idx < status->links.Size; link_idx++) {
    NodeLink *link = &status->links[link_idx];
    Node *node_inp = &status->nodes[link->InputIdx];
    Node *node_out = &status->nodes[link->OutputIdx];
    ImVec2 p1 = offset + node_inp->GetOutputSlotPos(link->InputSlot);
    ImVec2 p2 = offset + node_out->GetInputSlotPos(link->OutputSlot);
    draw_list->AddBezierCurve(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2,
                              IM_COL32(200, 200, 100, 255), 3.0f);
  }

  // Display nodes
  for (int node_idx = 0; node_idx < status->nodes.Size; node_idx++) {
    Node *node = &status->nodes[node_idx];
    ImGui::PushID(node->ID);
    ImVec2 node_rect_min = offset + node->Pos;

    // Display node contents first
    draw_list->ChannelsSetCurrent(1); // Foreground
    bool old_any_active = ImGui::IsAnyItemActive();
    ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
    ImGui::BeginGroup(); // Lock horizontal position
    ImGui::Text("%s", node->Name);
    // if you want any kind of content for the node you can add it here
    // ADD WIDGETS HERe
    ImGui::EndGroup();

    // Save the size of what we have emitted and whether any of the widgets are
    // being used
    bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
    node->Size =
        ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
    ImVec2 node_rect_max = node_rect_min + node->Size;

    // Display node box
    draw_list->ChannelsSetCurrent(0); // Background
    ImGui::SetCursorScreenPos(node_rect_min);
    ImGui::InvisibleButton("node", node->Size);
    if (ImGui::IsItemHovered()) {
      node_hovered_in_scene = node->ID;
      open_context_menu |= ImGui::IsMouseClicked(1);
    }
    bool node_moving_active = ImGui::IsItemActive();
    if (node_widgets_active || node_moving_active)
      status->node_selected = node->ID;
    if (node_moving_active && ImGui::IsMouseDragging(0))
      node->Pos = node->Pos + ImGui::GetIO().MouseDelta;

    ImU32 node_bg_color =
        (node_hovered_in_list == node->ID ||
         node_hovered_in_scene == node->ID ||
         (node_hovered_in_list == -1 && status->node_selected == node->ID))
            ? IM_COL32(75, 75, 75, 255)
            : IM_COL32(60, 60, 60, 255);
    draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
    draw_list->AddRect(node_rect_min, node_rect_max,
                       IM_COL32(100, 100, 100, 255), 4.0f);
    ImVec2 mousePos = ImGui::GetMousePos();
    for (int slot_idx = 0; slot_idx < node->InputsCount; slot_idx++) {

      ImVec2 pos = offset + node->GetInputSlotPos(slot_idx);
      ImVec2 delta = mousePos - pos;
      float dist = sqrtf((delta.x * delta.x) + (delta.y * delta.y));
      if (dist < NODE_SLOT_RADIUS * 1.1f) {
        draw_list->AddCircleFilled(pos, NODE_SLOT_RADIUS * 1.3f,
                                   IM_COL32(250, 150, 150, 150));

        // check how fare we are
        plugToolTip(node->node->getInputPlugs()[slot_idx].name.c_str());
      } else {
        draw_list->AddCircleFilled(pos, NODE_SLOT_RADIUS,
                                   IM_COL32(150, 150, 150, 150));
      }
    }
    for (int slot_idx = 0; slot_idx < node->OutputsCount; slot_idx++) {
      ImVec2 pos = offset + node->GetOutputSlotPos(slot_idx);

      ImVec2 delta = mousePos - pos;
      float dist = sqrtf((delta.x * delta.x) + (delta.y * delta.y));
      if (dist < NODE_SLOT_RADIUS * 1.1f) {
        draw_list->AddCircleFilled(pos, NODE_SLOT_RADIUS * 1.3f,
                                   IM_COL32(250, 150, 150, 150));
        // check how fare we are
        // ImGui::SetTooltip("out name");
        plugToolTip(node->node->getOutputPlugs()[slot_idx].name.c_str());

        // ImGui::SetTooltip();
        // ImGui::SetTooltip()
      } else {
        draw_list->AddCircleFilled(pos, NODE_SLOT_RADIUS,
                                   IM_COL32(150, 150, 150, 150));
      }
    }

    ImGui::PopID();
  }
  draw_list->ChannelsMerge();

  // Open context menu
  if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() &&
      ImGui::IsMouseClicked(1)) {
    status->node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
    open_context_menu = true;
  }
  if (open_context_menu) {
    ImGui::OpenPopup("context_menu");
    if (node_hovered_in_list != -1)
      status->node_selected = node_hovered_in_list;
    if (node_hovered_in_scene != -1)
      status->node_selected = node_hovered_in_scene;
  }

  // Draw context menu
  /*
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
  if (ImGui::BeginPopup("context_menu")) {
    Node *node = status->node_selected != -1 ?
  &status->nodes[status->node_selected] : NULL; ImVec2 scene_pos =
  ImGui::GetMousePosOnOpeningCurrentPopup() - offset; if (node) {
      ImGui::Text("Node '%s'", node->Name);
      ImGui::Separator();
      if (ImGui::MenuItem("Rename..", NULL, false, false)) {
      }
      if (ImGui::MenuItem("Delete", NULL, false, false)) {
      }
      if (ImGui::MenuItem("Copy", NULL, false, false)) {
      }
    } else {
       if (ImGui::MenuItem("Add")) {
        status->nodes.push_back(
            Node(status->nodes.Size, "New node", scene_pos, 2, 2,nullptr));
      }
      if (ImGui::MenuItem("Paste", NULL, false, false)) {
      }
    }
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();
  */

  // Scrolling
  if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
      ImGui::IsMouseDragging(2, 0.0f))
    status->scrolling = status->scrolling + ImGui::GetIO().MouseDelta;

  ImGui::PopItemWidth();
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
  ImGui::EndGroup();

  ImGui::End();
}

struct NodePosition {
  const GraphNode *node;
  float posAtRecursion;
  uint32_t parentIdx;
};
struct NodeQueue {
  const GraphNode *node;
  int parentIdx = -1;
};

RenderGraphWidget::~RenderGraphWidget() { delete status; }

void RenderGraphWidget::initialize(Graph *graph) {
  m_graph = graph;

  m_debugConfig.depthMin = 1.0f;
  m_debugConfig.depthMax = 0.0f;

  if (status != nullptr) {
    delete status;
  }
  status = new GraphStatus{};
  status->opened = false;

  float yStart = status->GRAPH_HEIGHT * 0.5f;
  float xMid = status->GRAPH_WIDTH - 100;
  const GraphNode *finalNode = graph->getFinalNode();

  std::vector<NodePosition> nodesToAdd;
  std::unordered_map<uint32_t, uint32_t> m_inputCountPerNode;
  std::unordered_set<uint32_t> visitedNodes;
  nodesToAdd.reserve(40);
  std::queue<NodeQueue> queue1;
  std::queue<NodeQueue> queue2;
  std::queue<NodeQueue> *currentQueue = &queue1;
  std::queue<NodeQueue> *nextQueue = &queue2;

  currentQueue->push(NodeQueue{finalNode, -1});
  bool go = true;
  while (go) {
    // this is the counter telling us for each recursion which node in the loop
    // we are processing, used for auto-layout
    uint32_t recCount = 0;
    while (!currentQueue->empty()) {
      // here we get first the current node from the queue
      // and build a node position, a structure with all the necessary
      // data for then being able to render the nodes.

      const NodeQueue &curr = currentQueue->front();
      // first we check whether or not the already processed the node
      if (visitedNodes.find(curr.node->getNodeIdx()) == visitedNodes.end()) {
        nodesToAdd.emplace_back(
            NodePosition{curr.node, static_cast<float>(recCount),
                         static_cast<uint32_t>(curr.parentIdx)});

        visitedNodes.insert(curr.node->getNodeIdx());

        // here we store each node the number of inputs it has, used for
        // automatic layout
        m_inputCountPerNode[curr.node->getNodeIdx()] =
            curr.node->getInputCount();
        recCount++;

        // lets process all the inputs, by accessing the other plug and getting
        // the parent node, the node will be added to the queue to be processed
        // in the next round
        const std::vector<Plug> &inPlugs = curr.node->getInputPlugs();
        for (size_t i = 0; i < inPlugs.size(); ++i) {
          // get the connections
          const std::vector<Plug *> *conns =
              curr.node->getPlugConnections(&inPlugs[i]);
          // if not empty we iterate all of them and extract the node at the
          // other end side
          if (conns != nullptr) {
            for (auto &conn : (*conns)) {

              status->links.push_back(
                  NodeLink(conn->nodePtr->getNodeIdx(), conn->index,
                           curr.node->getNodeIdx(), inPlugs[i].index));
              nextQueue->push(NodeQueue{
                  conn->nodePtr, static_cast<int>(curr.node->getNodeIdx())});
            }
          }
        }
      }
      currentQueue->pop();
    }

    go = !nextQueue->empty();
    std::swap(currentQueue, nextQueue);
  }
  float yStep = 100.0f;
  float xStep = 150.0f;

  std::unordered_map<uint32_t, ImVec2> posCache;

  status->nodes.resize(static_cast<int>(nodesToAdd.size()));
  for (size_t i = 0; i < nodesToAdd.size(); ++i) {

    // compute x position
    float xPos;
    float yPos;
    // if no parent aka parentIdx == -1 we just use the normal start and end
    if (nodesToAdd[i].parentIdx == -1) {
      xPos = xMid;
      yPos = yStart;
    } else {
      // if we have a parent then we are going to layout the node
      // we have a cached position for the parent, we are going to offset based
      // on that
      ImVec2 cacheValue = posCache[nodesToAdd[i].parentIdx];
      xPos = cacheValue.x - xStep;
      uint32_t parentIdx = nodesToAdd[i].parentIdx;
      // nodes are layout vertically in a way that the parent
      // will be sitting in the middle vertically
      // meaning half of the connections will be above the y of the node, half
      // will be below
      auto parentPlugConnections =
          static_cast<float>(m_inputCountPerNode[parentIdx]);
      yPos = cacheValue.y - ((parentPlugConnections - 1) * 0.5f * yStep) +
             yStep * nodesToAdd[i].posAtRecursion;
    }
    // creating the new cache position
    posCache[nodesToAdd[i].node->getNodeIdx()] = ImVec2{xPos, yPos};

    // creating a node for the graph
    status->nodes[nodesToAdd[i].node->getNodeIdx()] =
        Node(nodesToAdd[i].node->getNodeIdx(),
             nodesToAdd[i].node->getNodeName().c_str(), ImVec2(xPos, yPos),
             nodesToAdd[i].node->getInputCount(),
             nodesToAdd[i].node->getOutputCount(), nodesToAdd[i].node);
  }
}

void RenderGraphWidget::render() {

  bool generateDebugEvent = false;
  if (!ImGui::CollapsingHeader("Debug Frame", ImGuiTreeNodeFlags_DefaultOpen))
    return;
  const char *items[] = {"fullFrame", "gbuffer", "normalsBuffer",
                         "specularBuffer", "depth"};
  const bool debugLayerValueChanged =
      ImGui::Combo("Pass", &currentDebugLayer, items, IM_ARRAYSIZE(items));

  // lets render post process stack configuration
  if (ImGui::CollapsingHeader("Post process stack")) {
    PostProcessStack *stack = dynamic_cast<PostProcessStack *>(
        m_graph->findNodeOfType("PostProcessStack"));
    if (stack != nullptr) {
      const std::vector<PostProcessEffect *> &effects = stack->getEffects();
      for (const auto &effect : effects) {

        PostProcessTypeDebug type = getPostProcessDebugType(effect->getType());
        SE_CORE_INFO("Type {0}", (int)type);
        if (type == PostProcessTypeDebug::NONE) {
          continue;
        }
        if (ImGui::CollapsingHeader(effect->getName())) {
          switch (type) {
          case (PostProcessTypeDebug::GAMMA_TONE_MAPPING): {
            auto *typedEffect = (GammaAndToneMappingEffect *)(effect);
            GammaToneMappingConfig &config = typedEffect->getConfig();
            const bool exposure =
                ImGui::SliderFloat("exposure", &config.exposure, 0.0f, 10.0f);
            const bool gamma =
                ImGui::SliderFloat("gamma", &config.gamma, 0.0f, 10.0f);
            if (exposure | gamma) {
              typedEffect->setConfigDirty();
            }

            break;
          }
          default:;
          }
        }
      }
    }
  }

  if (debugLayerValueChanged) {
    // TODO use a stack allocator for this?
    auto *event = new DebugLayerChanged(currentDebugLayer);
    globals::APPLICATION->queueEventForEndOfFrame(event);
  }

  const bool pressed = ImGui::Button("show render graph");
  if (pressed) {
    status->opened = !status->opened;
  }
  renderImguiGraph(status);

  if (generateDebugEvent) {
    auto *event = new DebugRenderConfigChanged(m_debugConfig);
    globals::APPLICATION->queueEventForEndOfFrame(event);
  }
}

void RenderGraphWidget::showGraph(bool value) {
  if (status != nullptr) {
    status->opened = value;
  }
}
} // namespace debug
} // namespace SirEngine
