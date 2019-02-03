#pragma once
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/textureManager.h"
#include <dxgi1_4.h>

struct ID3D12Resource;
namespace SirEngine {
namespace dx12 {

class Texture2D;
class DepthTexture;

class SwapChain {
public:
  SwapChain() = default;
  ~SwapChain();
  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;

  bool initialize(HWND window, int width, int height);
  inline IDXGISwapChain *getSwapChain() { return m_swapChain; }
  bool resize(FrameCommand *command, int width, int height);

  inline D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferView() const {
    // return
    // m_swapChainBuffersResource[m_currentBackBuffer].getCPUDescriptor();
    return m_swapChainBuffersDescriptors[m_currentBackBuffer].cpuHandle;
  }
  // inline Texture2D *currentBackBufferTexture() const {
  //  return &m_swapChainBuffersResource[m_currentBackBuffer];
  //}
  inline TextureHandle currentBackBufferTexture() const {
    return m_swapChainBuffersHandles[m_currentBackBuffer];
  }
  //inline DepthTexture *getCurrentDepth() const { return m_depth; }

  inline D3D12_VIEWPORT *getViewport() { return &m_screenViewport; }
  inline D3D12_RECT *getScissorRect() { return &m_scissorRect; }
  inline void present() {
    m_swapChain->Present(0, 0);
    m_currentBackBuffer = (m_currentBackBuffer + 1) % FRAME_BUFFERS_COUNT;
  }
  void clearDepth() const {
    CURRENT_FRAME_RESOURCE->fc.commandList->ClearDepthStencilView(
        //m_depth->getCPUDescriptor(),
        m_swapChainDepthDescriptors.cpuHandle,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
  }
  inline D3D12_CPU_DESCRIPTOR_HANDLE getDepthCPUDescriptor() {
	  return m_swapChainDepthDescriptors.cpuHandle;
//    return m_depth->getCPUDescriptor();

    // return m_depthStencilBufferResource.cpuDescriptorHandle;
  }

private:
  // framebuffer configuration, hardcoded for the time being
  const DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
  const DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  bool m_4xMsaaState = false; // 4X MSAA enabled
  UINT m_msaaQuality;

  // Pointers to dx12 various interfaces
  IDXGISwapChain *m_swapChain = nullptr;

  UINT m_currentBackBuffer = 0;
  // Hard-coded double buffering for the time being

  //Texture2D *m_swapChainBuffersResource = nullptr;
  TextureHandle m_swapChainBuffersHandles[FRAME_BUFFERS_COUNT];
  DescriptorPair m_swapChainBuffersDescriptors[FRAME_BUFFERS_COUNT];

  TextureHandle m_swapChainDepth;
  DescriptorPair m_swapChainDepthDescriptors;

  //DepthTexture *m_depth = nullptr;

  D3D12_VIEWPORT m_screenViewport;
  D3D12_RECT m_scissorRect;
  bool m_isInit = false;
};
} // namespace dx12
} // namespace SirEngine
