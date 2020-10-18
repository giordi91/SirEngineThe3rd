#include "platform/windows/graphics/dx12/dx12ImguiManager.h"

#include "SirEngine/graphics/debugAnnotations.h"
#include "descriptorHeap.h"
#include "dx12SwapChain.h"
#include "imgui/imgui.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/imgui_impl_dx12.h"
#include "SirEngine/events/applicationEvent.h"

namespace SirEngine::dx12 {
void Dx12ImGuiManager::initialize() {
  // need to initialize ImGui dx12
  dx12::DescriptorPair pair;
  dx12::GLOBAL_CBV_SRV_UAV_HEAP->reserveDescriptor(pair);
  ImGui_ImplDX12_Init(dx12::DEVICE, FRAME_BUFFERS_COUNT,
                      DXGI_FORMAT_R8G8B8A8_UNORM, pair.cpuHandle,
                      pair.gpuHandle);
}

void Dx12ImGuiManager::cleanup() { ImGui_ImplDX12_Shutdown(); }

void Dx12ImGuiManager::startFrame() {
  annotateGraphicsBegin("UI Draw");
  TextureHandle destination = dx12::SWAP_CHAIN->currentBackBufferTexture();
  D3D12_RESOURCE_BARRIER barriers[1];
  int counter = dx12::TEXTURE_MANAGER->transitionTexture2DifNeeded(
      destination, D3D12_RESOURCE_STATE_RENDER_TARGET, barriers, 0);
  if (counter) {
    dx12::CURRENT_FRAME_RESOURCE->fc.commandList->ResourceBarrier(counter,
                                                                  barriers);
  }

  dx12::TEXTURE_MANAGER->bindBackBuffer();
  ImGui_ImplDX12_NewFrame();
}

void Dx12ImGuiManager::endFrame() {
  ImGui::Render();
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
                                dx12::CURRENT_FRAME_RESOURCE->fc.commandList);
  annotateGraphicsEnd();
}

void Dx12ImGuiManager::onResizeEvent(const WindowResizeEvent& e)
{
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(static_cast<float>(e.getWidth()),
                          static_cast<float>(e.getHeight()));
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
  ImGui_ImplDX12_InvalidateDeviceObjects();
	
}
}  // namespace SirEngine::dx12
