#include "SirEngine/debugUiWidgets/renderGraphWidget.h"
#include "SirEngine/graphics/nodeGraph.h"
#include "imgui/imgui.h"
#include <cmath>
#include <queue>

namespace SirEngine {
namespace debug {

struct Node {
  int ID;
  char Name[32];
  ImVec2 Pos, Size;
  // float   Value;
  // ImVec4  Color;
  int InputsCount, OutputsCount;

  Node(int id, const char *name, const ImVec2 &pos, int inputs_count,
       int outputs_count) {
    ID = id;
    strncpy(Name, name, 31);
    Name[31] = 0;
    Pos = pos;
    InputsCount = inputs_count;
    OutputsCount = outputs_count;
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
  const char *plugName;

  NodeLink(int input_idx, int input_slot, int output_idx, int output_slot,
           const char *inPlugName) {
    InputIdx = input_idx;
    InputSlot = input_slot;
    OutputIdx = output_idx;
    OutputSlot = output_slot;
    plugName = inPlugName;
  }
};

// NB: You can use math functions/operators on ImVec2 if you #define
// IMGUI_DEFINE_MATH_OPERATORS and #include "imgui_internal.h" Here we only
// declare simple +/- operators so others don't leak into the demo code.
static inline ImVec2 operator+(const ImVec2 &lhs, const ImVec2 &rhs) {
  return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}
static inline ImVec2 operator-(const ImVec2 &lhs, const ImVec2 &rhs) {
  return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

struct GrapStatus {
  ImVector<Node> nodes;
  ImVector<NodeLink> links;
  ImVec2 scrolling = ImVec2(0.0f, 0.0f);
  bool show_grid = true;
  int node_selected = -1;
  bool opened = true;
};

static GrapStatus status = GrapStatus{};

void renderImguiGraph() {
  // ImVector<Node> &nodes, ImVector<NodeLink> &links){
  // GrapStatus &status) {
  bool opened = true;
  ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiSetCond_FirstUseEver);
  if (!ImGui::Begin("Example: Custom Node Graph", &opened)) {
    ImGui::End();
    return;
  }

  // static ImVector<Node> nodes;
  // static ImVector<NodeLink> links;
  static bool inited = false;
  static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
  static bool show_grid = true;
  static int node_selected = -1;
  // if (!inited)
  //{
  //    nodes.push_back(Node(0, "MainTex", ImVec2(40, 50),   1, 1));
  //    nodes.push_back(Node(1, "BumpMap", ImVec2(40, 150),  1, 1));
  //    nodes.push_back(Node(2, "Combine", ImVec2(270, 80),  2, 2));
  //    links.push_back(NodeLink(0, 0, 2, 0));
  //    links.push_back(NodeLink(1, 0, 2, 1));
  //    inited = true;
  //}

  // Draw a list of nodes on the left side
  bool open_context_menu = false;
  int node_hovered_in_list = -1;
  int node_hovered_in_scene = -1;
  ImGui::BeginChild("node_list", ImVec2(100, 0));
  ImGui::Text("Nodes");
  ImGui::Separator();
  for (int node_idx = 0; node_idx < status.nodes.Size; node_idx++) {
    Node *node = &status.nodes[node_idx];
    ImGui::PushID(node->ID);
    if (ImGui::Selectable(node->Name, node->ID == node_selected))
      node_selected = node->ID;
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
  ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x,
              scrolling.y);
  ImGui::SameLine(ImGui::GetWindowWidth() - 100);
  ImGui::Checkbox("Show grid", &show_grid);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, IM_COL32(60, 60, 70, 200));
  ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
  ImGui::PushItemWidth(120.0f);

  ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling;
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  // Display grid
  if (show_grid) {
    ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
    float GRID_SZ = 64.0f;
    ImVec2 win_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetWindowSize();
    for (float x = fmodf(scrolling.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
      draw_list->AddLine(ImVec2(x, 0.0f) + win_pos,
                         ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
    for (float y = fmodf(scrolling.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
      draw_list->AddLine(ImVec2(0.0f, y) + win_pos,
                         ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
  }

  // Display links
  draw_list->ChannelsSplit(2);
  draw_list->ChannelsSetCurrent(0); // Background
  for (int link_idx = 0; link_idx < status.links.Size; link_idx++) {
    NodeLink *link = &status.links[link_idx];
    Node *node_inp = &status.nodes[link->InputIdx];
    Node *node_out = &status.nodes[link->OutputIdx];
    ImVec2 p1 = offset + node_inp->GetOutputSlotPos(link->InputSlot);
    ImVec2 p2 = offset + node_out->GetInputSlotPos(link->OutputSlot);
    draw_list->AddBezierCurve(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2,
                              IM_COL32(200, 200, 100, 255), 3.0f);
  }

  // Display nodes
  for (int node_idx = 0; node_idx < status.nodes.Size; node_idx++) {
    Node *node = &status.nodes[node_idx];
    ImGui::PushID(node->ID);
    ImVec2 node_rect_min = offset + node->Pos;

    // Display node contents first
    draw_list->ChannelsSetCurrent(1); // Foreground
    bool old_any_active = ImGui::IsAnyItemActive();
    ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
    ImGui::BeginGroup(); // Lock horizontal position
    ImGui::Text("%s", node->Name);
    // ImGui::SliderFloat("##value", &node->Value, 0.0f, 1.0f, "Alpha %.2f");
    // ImGui::ColorEdit3("##color", &node->Color.x);
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
      node_selected = node->ID;
    if (node_moving_active && ImGui::IsMouseDragging(0))
      node->Pos = node->Pos + ImGui::GetIO().MouseDelta;

    ImU32 node_bg_color =
        (node_hovered_in_list == node->ID ||
         node_hovered_in_scene == node->ID ||
         (node_hovered_in_list == -1 && node_selected == node->ID))
            ? IM_COL32(75, 75, 75, 255)
            : IM_COL32(60, 60, 60, 255);
    draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
    draw_list->AddRect(node_rect_min, node_rect_max,
                       IM_COL32(100, 100, 100, 255), 4.0f);
    ImVec2 mousePos = ImGui::GetMousePos();
    // ImVec2 mousePos = ImGui::GetCursorPos();
    for (int slot_idx = 0; slot_idx < node->InputsCount; slot_idx++) {

      ImVec2 pos = offset + node->GetInputSlotPos(slot_idx);
      ImVec2 delta = mousePos - pos;
      float dist = sqrtf((delta.x * delta.x) + (delta.y * delta.y));
      if (dist < NODE_SLOT_RADIUS * 1.1f) {
        draw_list->AddCircleFilled(pos, NODE_SLOT_RADIUS * 1.3f,
                                   IM_COL32(250, 150, 150, 150));

        // check how fare we are
        ImGui::SetTooltip("in name");
      } else {
        draw_list->AddCircleFilled(pos, NODE_SLOT_RADIUS,
                                   IM_COL32(150, 150, 150, 150));
      }
    }
    for (int slot_idx = 0; slot_idx < node->OutputsCount; slot_idx++) {
      ImVec2 pos = offset + node->GetOutputSlotPos(slot_idx);

      ImVec2 delta = mousePos - pos;
      float dist = sqrtf((delta.x *delta.x) + (delta.y *delta.y));
      if (dist < NODE_SLOT_RADIUS * 1.1f) {
        draw_list->AddCircleFilled(pos, NODE_SLOT_RADIUS * 1.3f,
                                   IM_COL32(250, 150, 150, 150));
        // check how fare we are
        ImGui::SetTooltip("out name");
      }
	  else
	  {
        draw_list->AddCircleFilled(pos, NODE_SLOT_RADIUS ,
                                   IM_COL32(150, 150, 150, 150));
		  
	  }
    }

    ImGui::PopID();
  }
  draw_list->ChannelsMerge();

  // Open context menu
  if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() &&
      ImGui::IsMouseClicked(1)) {
    node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
    open_context_menu = true;
  }
  if (open_context_menu) {
    ImGui::OpenPopup("context_menu");
    if (node_hovered_in_list != -1)
      node_selected = node_hovered_in_list;
    if (node_hovered_in_scene != -1)
      node_selected = node_hovered_in_scene;
  }

  // Draw context menu
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
  if (ImGui::BeginPopup("context_menu")) {
    Node *node = node_selected != -1 ? &status.nodes[node_selected] : NULL;
    ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
    if (node) {
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
        status.nodes.push_back(
            Node(status.nodes.Size, "New node", scene_pos, 2, 2));
      }
      if (ImGui::MenuItem("Paste", NULL, false, false)) {
      }
    }
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();

  // Scrolling
  if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
      ImGui::IsMouseDragging(2, 0.0f))
    scrolling = scrolling + ImGui::GetIO().MouseDelta;

  ImGui::PopItemWidth();
  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
  ImGui::EndGroup();

  ImGui::End();
}

struct NodePosition {
  const GraphNode *node;
  float recursionLevel;
  float posAtRecursion;
  uint32_t parentIdx;
};
struct NodeQueue {
  const GraphNode *node;
  int parentIdx = -1;
};

void RenderGraphWidget::initialize(Graph *graph) {
  float recursionLevel = 0.0f;
  float yStart = 250;
  float xMid = 450;
  const GraphNode *finalNode = graph->getFinalNode();

  std::vector<NodePosition> nodesToAdd;
  std::unordered_map<uint32_t, uint32_t> m_inputCountPerNode;
  nodesToAdd.reserve(40);
  std::queue<NodeQueue> queue1;
  std::queue<NodeQueue> queue2;
  std::queue<NodeQueue> *currentQueue = &queue1;
  std::queue<NodeQueue> *nextQueue = &queue2;

  currentQueue->push(NodeQueue{finalNode, -1});
  bool go = true;
  while (go) {
    uint32_t recCount = 0;
    while (!currentQueue->empty()) {
      const NodeQueue &curr = currentQueue->front();
      nodesToAdd.emplace_back(
          NodePosition{curr.node, recursionLevel, static_cast<float>(recCount),
                       static_cast<uint32_t>(curr.parentIdx)});
      m_inputCountPerNode[curr.node->getNodeIdx()] = curr.node->getInputCount();
      recCount++;

      // check if has any other connections in the inputs
      const std::vector<Plug> &inPlugs = curr.node->getInputPlugs();
      for (size_t i = 0; i < inPlugs.size(); ++i) {
        const std::vector<Plug *> *conns =
            curr.node->getPlugConnections(&inPlugs[i]);
        if (conns != nullptr) {
          for (auto &conn : (*conns)) {
            status.links.push_back(NodeLink(conn->nodePtr->getNodeIdx(), i,
                                            curr.node->getNodeIdx(),
                                            conn->index, ""));
            nextQueue->push(NodeQueue{
                conn->nodePtr, static_cast<int>(curr.node->getNodeIdx())});
          }
        }
      }
      currentQueue->pop();
    }

    recursionLevel += 1.0f;

    go = !nextQueue->empty();
    std::swap(currentQueue, nextQueue);
  }
  float yStep = 100.0f;
  float xStep = 100.0f;

  std::unordered_map<uint32_t, ImVec2> posCache;

  for (size_t i = 0; i < nodesToAdd.size(); ++i) {

    // compute x position
    float xPos;
    float yPos;
    if (nodesToAdd[i].parentIdx == -1) {
      xPos = xMid - (xStep * nodesToAdd[i].recursionLevel);
      yPos = yStart;
    } else {
      ImVec2 cacheValue = posCache[nodesToAdd[i].parentIdx];
      xPos = cacheValue.x - (xStep);
      uint32_t parentIdx = nodesToAdd[i].parentIdx;
      float nodesAtRecursion =
          static_cast<float>(m_inputCountPerNode[parentIdx]);
      yPos = cacheValue.y - ((nodesAtRecursion - 1) * 0.5 * yStep) +
             yStep * nodesToAdd[i].posAtRecursion;
    }
    posCache[nodesToAdd[i].node->getNodeIdx()] = ImVec2{xPos, yPos};

    status.nodes.push_back(Node(i, nodesToAdd[i].node->getNodeName(),
                                ImVec2(xPos, yPos),
                                nodesToAdd[i].node->getInputCount(),
                                nodesToAdd[i].node->getOutputCount()));
  }
}

void RenderGraphWidget::render() {

  // renderImguiGraph(nodes, links );
  renderImguiGraph();
}
} // namespace debug
} // namespace SirEngine
