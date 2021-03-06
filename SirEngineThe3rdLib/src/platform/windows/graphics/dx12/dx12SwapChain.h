#pragma once
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"
#include <dxgi1_4.h>

namespace SirEngine::dx12 {

class Dx12SwapChain {
public:
  Dx12SwapChain() = default;
  ~Dx12SwapChain();
  Dx12SwapChain(const Dx12SwapChain &) = delete;
  Dx12SwapChain &operator=(const Dx12SwapChain &) = delete;

  bool initialize(HWND window, int width, int height);
  inline IDXGISwapChain *getSwapChain() const { return m_swapChain; }
  bool resize(FrameCommand *command, int width, int height);

  inline D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferView() const {
    return m_swapChainBuffersDescriptors[m_currentBackBuffer].cpuHandle;
  }

  inline TextureHandle currentBackBufferTexture() const {
    return m_swapChainBuffersHandles[m_currentBackBuffer];
  }

  inline D3D12_VIEWPORT *getViewport() { return &m_screenViewport; }
  inline D3D12_RECT *getScissorRect() { return &m_scissorRect; }
  inline void present() {
    m_swapChain->Present(0, 0);
    m_currentBackBuffer = (m_currentBackBuffer + 1) % FRAME_BUFFERS_COUNT;
  }

public:
  static constexpr DXGI_FORMAT SWAP_CHAIN_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

private:
  // frame-buffer configuration, hard-coded for the time being
  const DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
  bool m_4xMsaaState = false;
  UINT m_msaaQuality;

  IDXGISwapChain *m_swapChain = nullptr;

  UINT m_currentBackBuffer = 0;
  TextureHandle m_swapChainBuffersHandles[FRAME_BUFFERS_COUNT];
  DescriptorPair m_swapChainBuffersDescriptors[FRAME_BUFFERS_COUNT];

  D3D12_VIEWPORT m_screenViewport;
  D3D12_RECT m_scissorRect;
  bool m_isInit = false;
};
} // namespace SirEngine
