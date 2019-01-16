#include "imguiLayer.h"
#include "imgui/imgui.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/imgui_impl_dx12.h"

namespace SirEngine {
void ImguiLayer::onAttach() {
  // need to initialize ImGui dx12

	/*
  assert(m_fontTextureDescriptor == nullptr);
  m_fontTextureDescriptor = new dx12::D3DBuffer();
  m_descriptorIndex = dx12::DX12Handles::globalCBVSRVUAVheap->reserveDescriptor(
      m_fontTextureDescriptor);

  ImGui_ImplDX12_Init(dx12::DX12Handles::device, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
                      m_fontTextureDescriptor->cpuDescriptorHandle,
                      m_fontTextureDescriptor->gpuDescriptorHandle);
					  */
}

void ImguiLayer::onDetach() {
	/*
  ImGui_ImplDX12_Shutdown();
  delete m_fontTextureDescriptor;
  m_fontTextureDescriptor == nullptr;
  */
}

void ImguiLayer::onUpdate() {
	/*
  ImGui_ImplDX12_NewFrame();

  ImGuiIO &io = ImGui::GetIO();
  IM_ASSERT(io.Fonts->IsBuilt() &&
            "Font atlas not built! It is generally built by the renderer "
            "back-end. Missing call to renderer _NewFrame() function? e.g. "
            "ImGui_ImplOpenGL3_NewFrame().");

  io.DisplaySize = ImVec2((float)(1280), (float)(720));

  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  ImGui::NewFrame();
  // ImGui::ShowDemoWindow(&show_demo_window);
  {
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and
                                   // append into it.

    ImGui::Text("This is some useful text."); // Display some text (you can use
                                              // a format strings too)
    ImGui::Checkbox(
        "Demo Window",
        &show_demo_window); // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_demo_window);

    ImGui::SliderFloat("float", &f, 0.0f,
                       1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3(
        "clear color",
        (float *)&clear_color); // Edit 3 floats representing a color

    if (ImGui::Button("Button")) // Buttons return true when clicked (most
                                 // widgets return true when edited/activated)
      counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    // Rendering
    //FrameContext *frameCtxt = WaitForNextFrameResources();
    //UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
    //frameCtxt->CommandAllocator->Reset();

    //D3D12_RESOURCE_BARRIER barrier = {};
    //barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    //barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    //barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
    //barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    //barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    //barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    //g_pd3dCommandList->Reset(frameCtxt->CommandAllocator, NULL);
    //g_pd3dCommandList->ResourceBarrier(1, &barrier);
    //g_pd3dCommandList->ClearRenderTargetView(
    //    g_mainRenderTargetDescriptor[backBufferIdx], (float *)&clear_color, 0,
    //    NULL);
    //g_pd3dCommandList->OMSetRenderTargets(
    //    1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
    //g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
    //ImGui::Render();
    //ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
    //barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    //barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    //g_pd3dCommandList->ResourceBarrier(1, &barrier);
    //g_pd3dCommandList->Close();

    //g_pd3dCommandQueue->ExecuteCommandLists(
    //    1, (ID3D12CommandList *const *)&g_pd3dCommandList);

    //g_pSwapChain->Present(1, 0); // Present with vsync
    //// g_pSwapChain->Present(0, 0); // Present without vsync

    //UINT64 fenceValue = g_fenceLastSignaledValue + 1;
    //g_pd3dCommandQueue->Signal(g_fence, fenceValue);
    //g_fenceLastSignaledValue = fenceValue;
    //frameCtxt->FenceValue = fenceValue;
  }
	*/
}

void ImguiLayer::onEvent(Event &event) {}
} // namespace SirEngine
