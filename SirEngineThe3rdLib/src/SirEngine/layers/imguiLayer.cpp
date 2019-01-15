#include "imguiLayer.h"
#include "imgui/imgui.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/imgui_impl_dx12.h"

namespace SirEngine {
void ImguiLayer::onAttach() {
  // need to initialize ImGui dx12

  assert(m_fontTextureDescriptor == nullptr);
  m_fontTextureDescriptor = new dx12::D3DBuffer();
  m_descriptorIndex = dx12::DX12Handles::globalCBVSRVUAVheap->reserveDescriptor(
      m_fontTextureDescriptor);

  ImGui_ImplDX12_Init(dx12::DX12Handles::device, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
                      m_fontTextureDescriptor->cpuDescriptorHandle,
                      m_fontTextureDescriptor->gpuDescriptorHandle);
}

void ImguiLayer::onDetach() {
  ImGui_ImplDX12_Shutdown();
  delete m_fontTextureDescriptor;
  m_fontTextureDescriptor == nullptr;
}

void ImguiLayer::onUpdate() {
  ImGui_ImplDX12_NewFrame();

  ImGuiIO &io = ImGui::GetIO();
  IM_ASSERT(io.Fonts->IsBuilt() &&
            "Font atlas not built! It is generally built by the renderer "
            "back-end. Missing call to renderer _NewFrame() function? e.g. "
            "ImGui_ImplOpenGL3_NewFrame().");

  io.DisplaySize =
      ImVec2((float)(1280), (float)(720));

  ImGui::NewFrame();
  bool show_demo_window = true;
  ImGui::ShowDemoWindow(&show_demo_window);
}

void ImguiLayer::onEvent(Event &event) {}
} // namespace SirEngine
