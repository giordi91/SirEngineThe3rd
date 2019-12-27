#include "SirEngine/debugUiWidgets/memoryPoolTrackerWidget.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/runtimeString.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>

namespace SirEngine::debug {

static ImVec4 COLORS[7] = {
    {0.537f, 0.941f, 0.0f, 1.0f},  {0.945f, 0.918, 0.0f, 1.0f},
    {1.0f, 0.765f, 0.463f, 1.0f},  {1.0f, 0.592f, 0.588f, 1.0f},
    {1.0f, 0.455f, 0.792f, 1.0f},  {0.89f, 0.349f, 0.992f, 1.0f},
    {0.494f, 0.494f, 0.882f, 1.0f}};

struct DrawAlloc {
  size_t m_size;
  size_t m_offset;
  const char *m_name;
};
struct DrawAllocInfo {
  uint32_t m_allocationCount;
  uint32_t m_freeAllocations;
  uint32_t m_wastedMemoryInBytes;
  uint32_t m_totalAllocationInBytes;
  uint32_t m_totalUsedAllocationInBytes;
};

void clusterAllocations(const ResizableVector<BufferRangeTracker> *allocs,
                        std::vector<DrawAlloc> &clustered, size_t minAlloc,
                        DrawAllocInfo &info) {
  int allocCount = allocs->size();
  info.m_allocationCount = allocCount;
  DrawAlloc cAlloc{};
  size_t accum = 0;
  const char *name = "";

  char numberBuffer[64];
  for (int i = 0; i < allocCount; ++i) {
    const BufferRangeTracker &current = allocs->getConstRef(i);
    if (!current.m_range.isValid()) {
      // this allocation is not valid,means is free
      info.m_freeAllocations++;
      continue;
    }
    if (accum == 0) {
      cAlloc.m_offset = current.m_range.m_offset;
    }

    accum += current.m_actualAllocSize;
    itoa(current.m_actualAllocSize, numberBuffer, 10);
    const char *sizeTemp = frameConcatenation(numberBuffer, " Bytes \n");
    const char *allocTemp = frameConcatenation("alloc: ", sizeTemp);

    info.m_totalAllocationInBytes += current.m_actualAllocSize;
    info.m_totalUsedAllocationInBytes += current.m_range.m_size;
    info.m_wastedMemoryInBytes +=
        current.m_actualAllocSize - current.m_range.m_size;

    name = frameConcatenation(name, allocTemp);

    if (accum > minAlloc) {
      cAlloc.m_size = accum;
      cAlloc.m_name = name;
      clustered.push_back(cAlloc);
      accum = 0;
      name = "";
    }
  }

  // if we are left to a minimum allocation we pad it
  if (accum < minAlloc && accum != 0) {
    cAlloc.m_size = minAlloc;
    cAlloc.m_name = name;
    clustered.push_back(cAlloc);
    accum = 0;
    name = "";
  }
}

void renderMemoryPoolTracker(
    const char *headerName, const size_t poolRangeInBytes,
    const ResizableVector<BufferRangeTracker> *allocs) {
  // render constant buffer pool
  if (!ImGui::CollapsingHeader(headerName, ImGuiTreeNodeFlags_DefaultOpen))
    return;

  ImGuiWindow *window = ImGui::GetCurrentWindow();
  const ImGuiID id = window->GetID("");
  if (window->SkipItems)
    return;

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;

  float frameWidth = ImGui::CalcItemWidth();

  ImVec2 pos = window->DC.CursorPos;
  ImVec2 size = ImGui::CalcItemSize(ImVec2(0.f, 0.f), frameWidth, 50);
  ImRect bb(pos, pos + size);
  ImGui::ItemSize(size, style.FramePadding.y);
  if (!ImGui::ItemAdd(bb, 0))
    return;

  // Render BG
  ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true,
                     style.FrameRounding);

  bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));

  int count = allocs->size();
  auto range = double(poolRangeInBytes * MB_TO_BYTE);

  double threshold = 1.0 / (bb.GetSize().x);
  size_t minAlloc = threshold * range;

  std::vector<DrawAlloc> clusteredAllocs;
  DrawAllocInfo info{};
  clusterAllocations(allocs, clusteredAllocs, minAlloc, info);

  const BufferRangeTracker &tracker = (*allocs)[0];
  double allocRation = double(tracker.m_range.m_size) / range;
  double allocOffset = double(tracker.m_range.m_offset) / range;

  for (size_t i = 0; i < clusteredAllocs.size(); ++i) {

    auto minB = bb.Min;
    minB.x = (clusteredAllocs[i].m_offset / range) * bb.GetSize().x + bb.Min.x;
    auto maxB = bb.Max;
    maxB.x = (clusteredAllocs[i].m_offset + clusteredAllocs[i].m_size) / range *
                 bb.GetSize().x +
             bb.Min.x;
    ImRect innerBB(minB, maxB);
    ImGui::RenderRectFilledRangeH(
        window->DrawList, bb, ImGui::GetColorU32(COLORS[i]),
        static_cast<float>(clusteredAllocs[i].m_offset / range),
        static_cast<float>((clusteredAllocs[i].m_offset + clusteredAllocs[i].m_size) / range),
        style.FrameRounding);

    const bool hovered = ImGui::ItemHoverable(bb, id);
    if (hovered && innerBB.Contains(g.IO.MousePos)) {
      ImGui::SetTooltip(clusteredAllocs[i].m_name);
    }
  }

  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << "Pool Size: " << range*BYTE_TO_MB
         << "MB\n"
         << "Allocation count: " << info.m_allocationCount
         << ".\nFree allocations: " << info.m_freeAllocations
         << ".\nTotal alloc: " << info.m_totalAllocationInBytes * BYTE_TO_MB
         << "MB  " << info.m_totalAllocationInBytes << "Bytes\n"
         << "Total Used alloc: "
         << info.m_totalUsedAllocationInBytes * BYTE_TO_MB << "MB  "
         << info.m_totalUsedAllocationInBytes << "Bytes\n"
         << "Total wasted alloc: " << info.m_wastedMemoryInBytes * BYTE_TO_MB
         << "MB  " << info.m_wastedMemoryInBytes << "Bytes\n";
  ImGui::Text(stream.str().c_str());
}
} // namespace SirEngine::debug
