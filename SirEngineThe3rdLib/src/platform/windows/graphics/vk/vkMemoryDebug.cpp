#include "platform/windows/graphics/vk/vkMemoryDebug.h"
#include "platform/windows/graphics/vk/vk.h"

#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/vk/volk.h"
#include <dxgi1_4.h>
#include <imgui/imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "vkConstantBufferManager.h"
#include <imgui/imgui_internal.h>
#include <iomanip>
#include <sstream>
#include <string>

namespace SirEngine::vk {

ImVec4 colors[7] = {
    {0.537f, 0.941f, 0.0f, 1.0f},  {0.945f, 0.918, 0.0f, 1.0f},
    {1.0f, 0.765f, 0.463f, 1.0f},  {1.0f, 0.592f, 0.588f, 1.0f},
    {1.0f, 0.455f, 0.792f, 1.0f},  {0.89f, 0.349f, 0.992f, 1.0f},
    {0.494f, 0.494f, 0.882f, 1.0f}};

static IDXGIAdapter3 *DXGI_ADAPTER = nullptr;
void updateDXAdapter() {
  // After obtaining VkPhysicalDevice of your choice:
  VkPhysicalDeviceIDProperties physDeviceIDProps = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
  VkPhysicalDeviceProperties2 physDeviceProps = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  physDeviceProps.pNext = &physDeviceIDProps;
  vkGetPhysicalDeviceProperties2(vk::PHYSICAL_DEVICE, &physDeviceProps);

  // from:
  // https://asawicki.info/news_1695_there_is_a_way_to_query_gpu_memory_usage_in_vulkan_-_use_dxgi.html
  IDXGIFactory4 *dxgiFactory = nullptr;
  CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

  IDXGIAdapter1 *tmpDxgiAdapter = nullptr;

  UINT adapterIndex = 0;
  while (dxgiFactory->EnumAdapters1(adapterIndex, &tmpDxgiAdapter) !=
         DXGI_ERROR_NOT_FOUND) {
    DXGI_ADAPTER_DESC1 desc;
    tmpDxgiAdapter->GetDesc1(&desc);
    if (memcmp(&desc.AdapterLuid, physDeviceIDProps.deviceLUID, VK_LUID_SIZE) ==
        0)
      tmpDxgiAdapter->QueryInterface(IID_PPV_ARGS(&DXGI_ADAPTER));
    tmpDxgiAdapter->Release();
    ++adapterIndex;
  }
}
uint32_t getTotalGpuMemoryInMB() {
  DXGI_ADAPTER_DESC desc;
  assert(SUCCEEDED(DXGI_ADAPTER->GetDesc(&desc)));
  return static_cast<uint32_t>(desc.DedicatedVideoMemory * 1e-6f);
}

uint32_t getUsedGpuMemoryInMB() {
  // get GPU memory
  DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
  DXGI_ADAPTER->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);
  return static_cast<uint32_t>(info.CurrentUsage * 1e-6f);
}

struct DrawAlloc {
  size_t m_size;
  size_t m_offset;
  const char *m_name;
};

void clusterAllocations(const ResizableVector<BufferRangeTracker> *allocs,
                        std::vector<DrawAlloc> &clustered, size_t minAlloc) {
  int allocCount = allocs->size();
  DrawAlloc cAlloc{};
  size_t accum = 0;
  const char *name = "";

  char numberBuffer[64];
  for (int i = 0; i < allocCount; ++i) {
    const BufferRangeTracker &current = allocs->getConstRef(i);
    if (accum == 0) {
      cAlloc.m_offset = current.m_range.m_offset;
    }

    accum += current.m_actualAllocSize;
    itoa(current.m_actualAllocSize, numberBuffer, 10);
    const char *sizeTemp = frameConcatenation(numberBuffer, " Bytes \n");
    const char *allocTemp = frameConcatenation("alloc: ", sizeTemp);

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

void renderImGuiMemoryWidget() {
  if (DXGI_ADAPTER == nullptr) {
    updateDXAdapter();
  }

  if (!ImGui::CollapsingHeader("Memory", ImGuiTreeNodeFlags_DefaultOpen))
    return;

  ImGui::PushItemWidth(-1);

  uint32_t dedicatedVideoMemory = getTotalGpuMemoryInMB();
  uint32_t usedVideoMemory = getUsedGpuMemoryInMB();

  const float totalGPUMemMB = static_cast<float>(dedicatedVideoMemory);
  const float usedGPUMemInMB = static_cast<float>(usedVideoMemory);
  const float ratio = usedGPUMemInMB / totalGPUMemMB;

  std::string overlay = std::to_string(ratio * 100.0f) + "%";

  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << "GPU Memory: used "
         << usedGPUMemInMB << "MB"
         << " available " << totalGPUMemMB << "MB";
  std::string gpuLableString = stream.str();
  ImGui::Text(gpuLableString.c_str());
  ImGui::ProgressBar(ratio, ImVec2(0.f, 0.f), overlay.c_str());

  //
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

  auto allocs = vk::CONSTANT_BUFFER_MANAGER->getAllocations();
  int count = allocs->size();
  auto range =
      double(VkConstantBufferManager::SLAB_ALLOCATION_IN_MB * MB_TO_BYTE);

  double threshold = 1.0 / (bb.GetSize().x);
  size_t minAlloc = threshold * range;

  std::vector<DrawAlloc> clusteredAllocs;
  clusterAllocations(allocs, clusteredAllocs, minAlloc);

  const BufferRangeTracker &tracker = (*allocs)[0];
  double allocRation = double(tracker.m_range.m_size) / range;
  double allocOffset = double(tracker.m_range.m_offset) / range;

  // double width = allocOffset + allocRation;

  for (size_t i = 0; i < clusteredAllocs.size(); ++i) {

      auto minB = bb.Min;
      minB.x = (clusteredAllocs[i].m_offset / range)*bb.GetSize().x +bb.Min.x;
      auto maxB = bb.Max;
      maxB.x = (clusteredAllocs[i].m_offset + clusteredAllocs[i].m_size)/range*bb.GetSize().x+bb.Min.x;
      ImRect inner_bb(minB,maxB);
    ImGui::RenderRectFilledRangeH(
        window->DrawList, bb, ImGui::GetColorU32(colors[i]),
        clusteredAllocs[i].m_offset / range,
        (clusteredAllocs[i].m_offset + clusteredAllocs[i].m_size) / range,
        style.FrameRounding);

    const bool hovered = ImGui::ItemHoverable(bb, id);
    if(hovered && inner_bb.Contains(g.IO.MousePos)) {
	    ImGui::SetTooltip(clusteredAllocs[i].m_name);
    }
  }

  // showing memory pools
  // constant buffer
  // ImGui::ShowDemoWindow();

  ImGui::PopItemWidth();
}
} // namespace SirEngine::vk
