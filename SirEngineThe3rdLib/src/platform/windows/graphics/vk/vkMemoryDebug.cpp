#include "platform/windows/graphics/vk/vkMemoryDebug.h"
#include "platform/windows/graphics/vk/vk.h"

#include "SirEngine/debugUiWidgets/memoryPoolTrackerWidget.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/vk/volk.h"
#include "vkConstantBufferManager.h"
#include <dxgi1_4.h>
#include <imgui/imgui.h>
#include <iomanip>
#include <sstream>
#include <string>

namespace SirEngine::vk {

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
  DXGI_ADAPTER_DESC desc{};
  bool res = SUCCEEDED(DXGI_ADAPTER->GetDesc(&desc));
  assert(res);
  return static_cast<uint32_t>(desc.DedicatedVideoMemory * 1e-6f);
}

uint32_t getUsedGpuMemoryInMB() {
  // get GPU memory
  DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
  DXGI_ADAPTER->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);
  return static_cast<uint32_t>(info.CurrentUsage * 1e-6f);
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

  const auto totalGPUMemMB = static_cast<float>(dedicatedVideoMemory);
  const auto usedGPUMemInMB = static_cast<float>(usedVideoMemory);
  const float ratio = usedGPUMemInMB / totalGPUMemMB;

  std::string overlay = std::to_string(ratio * 100.0f) + "%";

  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << "GPU Memory: used "
         << usedGPUMemInMB << "MB"
         << " available " << totalGPUMemMB << "MB";
  std::string gpuLableString = stream.str();
  ImGui::Text(gpuLableString.c_str());
  ImGui::ProgressBar(ratio, ImVec2(0.f, 0.f), overlay.c_str());

  // render constant buffer pool
  size_t allocRange = VkConstantBufferManager::SLAB_ALLOCATION_IN_MB;
  debug::renderMemoryPoolTracker("Constant buffer pool", allocRange,
                                 vk::CONSTANT_BUFFER_MANAGER->getAllocations());

  // ImGui::ShowDemoWindow();

  ImGui::PopItemWidth();
}
} // namespace SirEngine::vk
