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
  assert(SUCCEEDED(ADAPTER->GetDesc(&desc)));
  return static_cast<uint32_t>(desc.DedicatedVideoMemory * 1e-6f);
}

uint32_t getUsedGpuMemoryMB() {
  // get GPU memory
  DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
  dx12::ADAPTER->QueryVideoMemoryInfo(
      0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);
  return static_cast<uint32_t>(info.CurrentUsage * 1e-6f);
}

void renderImGuiMemoryWidget() {
  if (!ImGui::CollapsingHeader("Memory", ImGuiTreeNodeFlags_DefaultOpen))
    return;

  ImGui::PushItemWidth(-1);

  uint32_t totalMemory = getTotalGpuMemoryMB();
  uint32_t usedMemory = getUsedGpuMemoryMB();

  float totalGPUMemMB = static_cast<float>(totalMemory) ;
  float usedGPUMemInMB = static_cast<float>(usedMemory) ;
  float ratio = usedGPUMemInMB / totalGPUMemMB;

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
