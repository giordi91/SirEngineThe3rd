
#include "platform/windows/graphics/dx12/swapChain.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <d3d12.h>

namespace SirEngine {
namespace dx12 {
SwapChain::~SwapChain() {
  if (m_swapChainBuffersResource != nullptr) {
    delete m_swapChainBuffersResource;
    m_swapChainBuffersResource = nullptr;
  }

  // freeing depth and re-creating it;
  if (m_depth != nullptr) {
    delete m_depth;
    m_depth = nullptr;
  }
}
bool SwapChain::initialize(HWND window, int width, int height) {

  // Check 4X MSAA quality support for our back buffer format.
  // All Direct3D 11 capable devices support 4X MSAA for all render
  // target formats, so we only need to check quality support.
  D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
  msQualityLevels.Format = m_backBufferFormat;
  msQualityLevels.SampleCount = 4;
  msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
  msQualityLevels.NumQualityLevels = 0;
  HRESULT result = DX12Handles::device->CheckFeatureSupport(
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

  result = DX12Handles::dxiFactory->CreateSwapChain(DX12Handles::commandQueue,
                                                    &swapDesc, &m_swapChain);

  if (FAILED(result)) {
    return false;
  }
  return true;
}

bool SwapChain::resize(CommandList *command, int width, int height) {

  // Flush before changing any resources.
  // FlushCommandQueue();

  // HRESULT result = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
  resetCommandList(command);
  if (m_swapChainBuffersResource != nullptr) {
    delete m_swapChainBuffersResource;
    m_swapChainBuffersResource = nullptr;
  }

  m_swapChainBuffersResource = new Texture2D[m_swapChainBufferCount];

  // freeing depth and re-creating it;
  if (m_depth != nullptr) {
    delete m_depth;
    m_depth = nullptr;
  }
  m_depth = new DepthTexture();
  m_depth->initialize(width, height);

  // Resize the swap chain.
  HRESULT result = m_swapChain->ResizeBuffers(
      m_swapChainBufferCount, width, height, m_backBufferFormat,
      DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
  assert(SUCCEEDED(result) && "failed to resize swap chain");

  // resetting the current back buffer
  m_currentBackBuffer = 0;

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(
      DX12Handles::globalRTVheap->getCPUStart());

  ID3D12Resource *resource;
  for (UINT i = 0; i < m_swapChainBufferCount; i++) {
    HRESULT res = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&resource));

    m_swapChainBuffersResource[i].initializeRTFromResource(resource);
  }

  // Transition the resource from its initial state to be used as a depth
  // buffer.

  command->commandList->ResourceBarrier(
      1, &CD3DX12_RESOURCE_BARRIER::Transition(
             m_depth->getResource(), D3D12_RESOURCE_STATE_COMMON,
             D3D12_RESOURCE_STATE_DEPTH_WRITE));

  // Execute the resize commands.
  executeCommandList(DX12Handles::commandQueue, command);

  // Wait until resize is complete.
  flushCommandQueue(DX12Handles::commandQueue);

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
} // namespace dx12
} // namespace SirEngine
