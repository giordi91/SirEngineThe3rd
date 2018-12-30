/*
#include "Illuminati/rendering/swapChain.h"
#include "Illuminati/rendering/texture2D.h"
#include "Illuminati/system/DXInterfaces.h"
#include <d3d12.h>
namespace dx12 {
namespace rendering {

bool SwapChain::initialize(system::DXInterfaces *interfaces, HWND window,
                           int width, int height) {

  m_dxInterfaces = interfaces;
  // Check 4X MSAA quality support for our back buffer format.
  // All Direct3D 11 capable devices support 4X MSAA for all render
  // target formats, so we only need to check quality support.
  D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
  msQualityLevels.Format = m_backBufferFormat;
  msQualityLevels.SampleCount = 4;
  msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
  msQualityLevels.NumQualityLevels = 0;
  HRESULT result = interfaces->getDevice()->CheckFeatureSupport(
      D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels,
      sizeof(msQualityLevels));

  m_msaaQuality = msQualityLevels.NumQualityLevels;
  assert(m_msaaQuality > 0 && "Unexpected MSAA quality level.");

  // Release previous swap chain
  DXGI_SWAP_CHAIN_DESC swapDesc;
  swapDesc.BufferDesc.Width = width;
  swapDesc.BufferDesc.Height = height;
  swapDesc.BufferDesc.RefreshRate.Numerator = 60;
  swapDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  // if we got a valid msaa state we set 4 samples
  swapDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
  swapDesc.SampleDesc.Quality = m_4xMsaaState ? m_msaaQuality - 1 : 0;
  swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  // double buffering
  swapDesc.BufferCount = 2;
  swapDesc.OutputWindow = window;
  swapDesc.Windowed = 1;
  swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  // the reason why we pass a queue is because when the swap chain flushes uses
  // the queue
  auto *factory = interfaces->getDXGIFactory();
  auto *queue = interfaces->getCommandQueue();
  result = factory->CreateSwapChain(queue->getCommandQueue(), &swapDesc,
                                    &m_swapChain);

  if (FAILED(result)) {
    return false;
  }
  return true;
}

bool SwapChain::resize(int width, int height) {
  // Flush before changing any resources.
  // FlushCommandQueue();

  // HRESULT result = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
  auto *queue = m_dxInterfaces->getCommandQueue();
  queue->resetCommandList();

  // Release the previous resources we will be recreating.
  for (int i = 0; i < m_swapChainBufferCount; ++i) {
    m_swapChainBuffersResource[i].clear();
  }
  // if (m_depthStencilBufferResource.resource) {
  //  m_depthStencilBufferResource.resource->Release();
  //}
  m_depth.clear();

  // Resize the swap chain.
  HRESULT result = m_swapChain->ResizeBuffers(
      m_swapChainBufferCount, width, height, m_backBufferFormat,
      DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

  // resetting the current back buffer
  m_currentBackBuffer = 0;

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(
      m_dxInterfaces->getRtvHeap()->getCPUStart());

  auto *device = m_dxInterfaces->getDevice();
  ID3D12Resource *resource;
  for (UINT i = 0; i < m_swapChainBufferCount; i++) {
    HRESULT res = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&resource));

    m_swapChainBuffersResource[i].initializeRTFromResource(
        m_dxInterfaces, resource, width, height);

    // createRTVSRV(m_dxInterfaces->getRtvHeap(), m_dxInterfaces->getDevice(),
    //             &m_swapChainBuffersResource[i]);
  }


  // Transition the resource from its initial state to be used as a depth
  // buffer.
  m_depth.initialize(m_dxInterfaces, width, height);

  queue->getCommandList()->ResourceBarrier(
      1, &CD3DX12_RESOURCE_BARRIER::Transition(
             m_depth.getResource(), D3D12_RESOURCE_STATE_COMMON,
             D3D12_RESOURCE_STATE_DEPTH_WRITE));

  // Execute the resize commands.
  queue->executeCommandList();

  // Wait until resize is complete.
  queue->flushCommandQueue();

  // Update the viewport transform to cover the client area.
  m_screenViewport.TopLeftX = 0;
  m_screenViewport.TopLeftY = 0;
  m_screenViewport.Width = static_cast<float>(width);
  m_screenViewport.Height = static_cast<float>(height);
  m_screenViewport.MinDepth = 0.0f;
  m_screenViewport.MaxDepth = 1.0f;

  m_scissorRect = {0, 0, width, height};
  return true;
}
} // namespace rendering
} // namespace dx12
*/
