#include "platform/windows/graphics/dx12/memoryDebug.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/dx12Adapter.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <imgui/imgui.h>
#include <iomanip>
#include <sstream>
namespace SirEngine {
namespace dx12 {
uint32_t getTotalGpuMemoryMB() {
  DXGI_ADAPTER_DESC desc;
  assert(SUCCEEDED(dx12::ADAPTER->getAdapter()->GetDesc(&desc)));
  return static_cast<uint32_t>(desc.DedicatedVideoMemory * 1e-6f);
}

uint32_t getUsedGpuMemoryMB() {
  // get GPU memory
  DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
  dx12::ADAPTER->getAdapter()->QueryVideoMemoryInfo(
      0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);
  return static_cast<uint32_t>(info.CurrentUsage * 1e-6f);
}

void renderImGuiMemoryWidget() {
  if (!ImGui::CollapsingHeader("Memory", ImGuiTreeNodeFlags_DefaultOpen))
    return;

  ImGui::PushItemWidth(-1);
  // CUSTOM WIDGET
  // void ImDrawList::AddCircle(const ImVec2& centre, float radius, ImU32 col,
  // int num_segments, float thickness)
  //{
  //    if ((col & IM_COL32_A_MASK) == 0)
  //        return;
  //
  //    const float a_max = IM_PI*2.0f * ((float)num_segments - 1.0f) /
  //    (float)num_segments; PathArcTo(centre, radius-0.5f, 0.0f, a_max,
  //    num_segments); PathStroke(col, true, thickness);

  // get gpu memory
  DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
  dx12::ADAPTER->getAdapter()->QueryVideoMemoryInfo(
      0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);

  DXGI_ADAPTER_DESC desc;
  SUCCEEDED(dx12::ADAPTER->getAdapter()->GetDesc(&desc));

  float totalGPUMemGB = static_cast<float>(desc.DedicatedVideoMemory) * 1e-9f;
  float usedGPUMemInGB = static_cast<float>(info.CurrentUsage) * 1e-9f;
  float totalGPUMemMB = static_cast<float>(desc.DedicatedVideoMemory) * 1e-6f;
  float usedGPUMemInMB = static_cast<float>(info.CurrentUsage) * 1e-6f;
  float ratio = usedGPUMemInGB / totalGPUMemGB;

  std::string overlay = std::to_string(ratio * 100.0f) + "%";

  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << "GPU Memory: used "
         << usedGPUMemInMB << "MB"
         << " available " << totalGPUMemMB << "MB";
  std::string gpuLableString = stream.str();
  ImGui::Text(gpuLableString.c_str());
  ImGui::ProgressBar(ratio, ImVec2(0.f, 0.f), overlay.c_str());

  // Heaps
  if (!ImGui::CollapsingHeader("Descriptor Heaps",
                               ImGuiTreeNodeFlags_DefaultOpen))
    return;
  if (!ImGui::CollapsingHeader("Global Heaps", ImGuiTreeNodeFlags_DefaultOpen))
    return;

  // CBV heap
  auto heapSize =
      static_cast<uint32_t>(dx12::GLOBAL_CBV_SRV_UAV_HEAP->getHeapSize());
  auto allocated = static_cast<uint32_t>(
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->getAllocatedDescriptorsCount());
  auto freeHandles = static_cast<uint32_t>(
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->getFreeHandleCount());

  float heapRatio =
      static_cast<float>(allocated) / static_cast<float>(heapSize);
  stream.str("");
  stream.clear();
  stream << std::fixed << std::setprecision(2) << "CBV_SRV_UAV heap: used "
         << allocated << "/" << heapSize << " "
         << "free handles :" << freeHandles;
  std::string heapLabel = stream.str();
  ImGui::Text(heapLabel.c_str());
  std::string overlayHeap = std::to_string(heapRatio * 100.0f) + "%";
  ImGui::ProgressBar(heapRatio, ImVec2(0.f, 0.f), overlayHeap.c_str());

  // DSV heap

  heapSize = static_cast<uint32_t>(dx12::GLOBAL_DSV_HEAP->getHeapSize());
  allocated = static_cast<uint32_t>(
      dx12::GLOBAL_DSV_HEAP->getAllocatedDescriptorsCount());
  freeHandles =
      static_cast<uint32_t>(dx12::GLOBAL_DSV_HEAP->getFreeHandleCount());

  heapRatio = static_cast<float>(allocated) / static_cast<float>(heapSize);
  stream.str("");
  stream.clear();
  stream << std::fixed << std::setprecision(2) << "DSV heap: used " << allocated
         << "/" << heapSize << " "
         << "free handles :" << freeHandles;
  heapLabel = stream.str();
  ImGui::Text(heapLabel.c_str());
  overlayHeap = std::to_string(heapRatio * 100.0f) + "%";
  ImGui::ProgressBar(heapRatio, ImVec2(0.f, 0.f), overlayHeap.c_str());

  // RTV heap
  heapSize = static_cast<uint32_t>(dx12::GLOBAL_RTV_HEAP->getHeapSize());
  allocated = static_cast<uint32_t>(
      dx12::GLOBAL_RTV_HEAP->getAllocatedDescriptorsCount());
  freeHandles =
      static_cast<uint32_t>(dx12::GLOBAL_RTV_HEAP->getFreeHandleCount());

  heapRatio = static_cast<float>(allocated) / static_cast<float>(heapSize);
  stream.str("");
  stream.clear();
  stream << std::fixed << std::setprecision(2) << "RTV heap: used " << allocated
         << "/" << heapSize << " "
         << "free handles :" << freeHandles;
  heapLabel = stream.str();
  ImGui::Text(heapLabel.c_str());
  overlayHeap = std::to_string(heapRatio * 100.0f) + "%";
  ImGui::ProgressBar(heapRatio, ImVec2(0.f, 0.f), overlayHeap.c_str());
  ImGui::PopItemWidth();
}
} // namespace dx12
} // namespace SirEngine
