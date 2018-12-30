/*
#pragma once
#include "Illuminati/rendering/depthTexture.h"
#include "Illuminati/rendering/texture2D.h"
#include "Illuminati/system/DXInterfaces.h"
#include "Illuminati/system/d3dBuffer.h"
#include <dxgi1_4.h>

struct ID3D12Resource;
namespace dx12 {
namespace system {
class DXInterfaces;
}
namespace rendering {

class SwapChain {
public:
  SwapChain() = default;
  ~SwapChain() = default;
  bool initialize(system::DXInterfaces *interfaces, HWND window, int width,
                  int height);
  inline IDXGISwapChain *getSwapChain() { return m_swapChain; }
  bool resize(int width, int height);

  inline D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferView() {
    return m_swapChainBuffersResource[m_currentBackBuffer].getCPUDescriptor();
  }
  inline Texture2D *currentBackBufferTexture() {
    return &m_swapChainBuffersResource[m_currentBackBuffer];
  }
  inline DepthTexture* getCurrentDepth()
  {
	  return &m_depth;
  }
  inline Texture2D *currentBackBuffer() {
    return &m_swapChainBuffersResource[m_currentBackBuffer];
  };

  inline D3D12_VIEWPORT *getViewport() { return &m_screenViewport; }
  inline D3D12_RECT *getScissorRect() { return &m_scissorRect; }
  inline void present() {
    m_swapChain->Present(0, 0);
    m_currentBackBuffer = (m_currentBackBuffer + 1) % m_swapChainBufferCount;
  }
  void clearDepth() {
    m_dxInterfaces->getCommandQueue()->getCommandList()->ClearDepthStencilView(
        m_depth.getCPUDescriptor(),
        // m_depthStencilBufferResource.cpuDescriptorHandle,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
  }
  inline D3D12_CPU_DESCRIPTOR_HANDLE getDepthCPUDescriptor() {
    return m_depth.getCPUDescriptor();
    // return m_depthStencilBufferResource.cpuDescriptorHandle;
  }

private:
  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;

private:
  // framebuffer config
  const DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
  const DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  bool m_4xMsaaState = false; // 4X MSAA enabled
  UINT m_msaaQuality;

  // Pointers to dx12 various interfaces
  system::DXInterfaces *m_dxInterfaces = nullptr;
  IDXGISwapChain *m_swapChain = nullptr;

  UINT m_currentBackBuffer = 0;
  // Hard-coded double buffering for the time being
  static const UINT m_swapChainBufferCount = 2;

  ID3D12Resource *m_swapChainBuffers[m_swapChainBufferCount];
  // system::D3DBuffer m_swapChainBuffersResource[m_swapChainBufferCount];
  Texture2D m_swapChainBuffersResource[m_swapChainBufferCount];
  // system::D3DBuffer m_depthStencilBufferResource;
  DepthTexture m_depth;

  D3D12_VIEWPORT m_screenViewport;
  D3D12_RECT m_scissorRect;
};

} // namespace rendering
} // namespace dx12
*/
