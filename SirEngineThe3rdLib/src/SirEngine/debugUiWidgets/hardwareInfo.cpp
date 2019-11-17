#include "SirEngine/debugUiWidgets/hardwareInfo.h"
#include "SirEngine/globals.h"
#include <cmath>
#include <imgui/imgui.h>
#include <iomanip>
#include <sstream>
#include <string>

#include "platform/windows/graphics/dx12/dx12Adapter.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/hardware/amd/amdGPUquery.h"

namespace SirEngine::debug {
HWInfoWidget::HWInfoWidget() {
  for (int i = 0; i < NUMBER_OF_SAMPLES; ++i) {
    temperatureSamples[i] = 0.0f;
    coreClockSamples[i] = 0.0f;
    memClockSamples[i] = 0.0f;
    usageSamples[i] = 0.0f;
  }

#ifdef BUILD_AMD
  gpuQuery = new Hardware::AMDGPUQuery();
  gpuQuery->initialize(100);
#endif
}

void HWInfoWidget::render() {

#ifdef BUILD_AMD
  // update the timings
  DXGI_ADAPTER_DESC desc;
  dx12::ADAPTER->getAdapter()->GetDesc(&desc);

  char t[128];
  size_t converted = 0;
  wcstombs_s(&converted, t, desc.Description, 128);
  ImGui::Text(t);

  gpuQuery->update();

  temperatureSamples[runningCounter] = gpuQuery->m_temp;
  coreClockSamples[runningCounter] = static_cast<float>(gpuQuery->m_coreFreq);
  memClockSamples[runningCounter] = static_cast<float>(gpuQuery->m_memFreq);
  usageSamples[runningCounter] = static_cast<float>(gpuQuery->m_usage);

  std::stringstream stream;
  std::string s;
  if (ImGui::CollapsingHeader("Core usage %:",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::PushItemWidth(ImGui::GetWindowWidth() - 90);
    //\n are tricks to try to place the bottom scale in the right place
    stream << 100 << "%\n\n\n\n\n" << 0 << "%";
    s = stream.str();
    ImGui::PlotLines(s.c_str(), usageSamples, IM_ARRAYSIZE(usageSamples),
                     runningCounter, "", 0.0f, 100.0f, ImVec2(0, 80));
  }

  if (ImGui::CollapsingHeader("Temperature:")) {
    ImGui::PushItemWidth(ImGui::GetWindowWidth() - 90);

    //\n are tricks to try to place the bottom scale in the right place
    stream.clear();
    stream.str("");
    stream << 100 << "C\n\n\n" << 0 << "C";
    s = stream.str();
    ImGui::PlotLines(s.c_str(), temperatureSamples,
                     IM_ARRAYSIZE(temperatureSamples), runningCounter, "", 0.0f,
                     100.0f, ImVec2(0, 60));
  }

  if (ImGui::CollapsingHeader("Core clocks:")) {
    ImGui::PushItemWidth(ImGui::GetWindowWidth() - 90);

    //\n are tricks to try to place the bottom scale in the right place
    stream.clear();
    stream.str("");
    stream << gpuQuery->m_maxCoreFreq << "mhz\n\n\n"
           << gpuQuery->m_minCoreFreq << "mhz";
    s = stream.str();
    ImGui::PlotLines(
        s.c_str(), coreClockSamples, IM_ARRAYSIZE(coreClockSamples),
        runningCounter, "", static_cast<float>(gpuQuery->m_minCoreFreq),
        static_cast<float>(gpuQuery->m_maxCoreFreq), ImVec2(0, 60));
  }

  if (ImGui::CollapsingHeader("Memory clocks:")) {
    ImGui::PushItemWidth(ImGui::GetWindowWidth() - 90);
    //\n are tricks to try to place the bottom scale in the right place
    stream.clear();
    stream.str("");
    stream << gpuQuery->m_maxMemFreq << "mhz\n\n\n"
           << gpuQuery->m_minMemFreq << "mhz";
    s = stream.str();
    ImGui::PlotLines(s.c_str(), memClockSamples, IM_ARRAYSIZE(memClockSamples),
                     runningCounter, "",
                     static_cast<float>(gpuQuery->m_minMemFreq),
                     static_cast<float>(gpuQuery->m_maxMemFreq), ImVec2(0, 60));
  }

  runningCounter = ((runningCounter + 1) % NUMBER_OF_SAMPLES);
#endif
}
} // namespace SirEngine::debug
