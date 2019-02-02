
#include "platform/windows/graphics/dx12/swapChain.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <d3d12.h>
#include <iostream>

namespace SirEngine {
namespace dx12 {

namespace SwapChainConstants {
const char *BACK_BUFFER_NAMES[3]{"backBuffer1", "backBuffer2", "backBuffer3"};

}

SwapChain::~SwapChain() {
  // if (m_swapChainBuffersResource != nullptr) {
  //  delete m_swapChainBuffersResource;
  //  m_swapChainBuffersResource = nullptr;
  //}
  for (int i = 0; i < FRAME_BUFFERS_COUNT; ++i) {
    dx12::TEXTURE_MANAGER->freeRTV(m_swapChainBuffersHandles[i],
                                   m_swapChainBuffersDescriptors[i]);
	dx12::TEXTURE_MANAGER->free(m_swapChainBuffersHandles[i]);
  }

  // freeing depth and re-creating it;
  if (m_depth != nullptr) {
    delete m_depth;
    m_depth = nullptr;
  }
}
bool SwapChain::initialize(const HWND window, const int width,
                           const int height) {

  // Check 4X MSAA quality support for our back buffer format.
  // All Direct3D 11 capable devices support 4X MSAA for all render
  // target formats, so we only need to check quality support.
  D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
  msQualityLevels.Format = m_backBufferFormat;
  msQualityLevels.SampleCount = 4;
  msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
  msQualityLevels.NumQualityLevels = 0;
  HRESULT result =
      DEVICE->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                  &msQualityLevels, sizeof(msQualityLevels));

  m_msaaQuality = msQualityLevels.NumQualityLevels;
  assert(m_msaaQuality > 0 && "Unexpected MSAA quality level.");

  // Release previous swap chain
  DXGI_SWAP_CHAIN_DESC swapDesc;
  swapDesc.BufferDesc.Width = width;
  swapDesc.BufferDesc.Height = height;
  swapDesc.BufferDesc.RefreshRate.Numerator = 0;
  swapDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  // if we got a valid MSAA state we set 4 samples
  swapDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
  swapDesc.SampleDesc.Quality = m_4xMsaaState ? m_msaaQuality - 1 : 0;
  swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  // double buffering
  swapDesc.BufferCount = FRAME_BUFFERS_COUNT;
  swapDesc.OutputWindow = window;
  swapDesc.Windowed = 1;
  swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  // the reason why we pass a queue is because when the swap chain flushes uses
  // the queue
  result = DXGI_FACTORY->CreateSwapChain(GLOBAL_COMMAND_QUEUE, &swapDesc,
                                         &m_swapChain);

  return FAILED(result);
}

bool SwapChain::resize(FrameCommand *command, const int width,
                       const int height) {

  // Flush before changing any resources.
  flushCommandQueue(GLOBAL_COMMAND_QUEUE);
  resetCommandList(command);

  if (m_isInit)
  {
	  for (int i = 0; i < FRAME_BUFFERS_COUNT; ++i) {
		  dx12::TEXTURE_MANAGER->freeRTV(m_swapChainBuffersHandles[i],
			  m_swapChainBuffersDescriptors[i]);
		  dx12::TEXTURE_MANAGER->free(m_swapChainBuffersHandles[i]);
	  }
  }

  // if (m_swapChainBuffersResource != nullptr) {
  //  for (int i = 0; i < FRAME_BUFFERS_COUNT; ++i) {
  //    m_swapChainBuffersResource[i].clear();
  //  }
  //  delete[] m_swapChainBuffersResource;
  //  m_swapChainBuffersResource = nullptr;
  //}

  // m_swapChainBuffersResource = new Texture2D[FRAME_BUFFERS_COUNT];

  // Resize the swap chain.
  HRESULT result = m_swapChain->ResizeBuffers(
      FRAME_BUFFERS_COUNT, width, height, m_backBufferFormat,
      DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
  assert(SUCCEEDED(result) && "failed to resize swap chain");

  // resetting the current back buffer
  m_currentBackBuffer = 0;

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(GLOBAL_RTV_HEAP->getCPUStart());

  for (UINT i = 0; i < FRAME_BUFFERS_COUNT; i++) {
    ID3D12Resource *resource;
    HRESULT res = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&resource));

    // m_swapChainBuffersResource[i].initializeRTFromResource(resource);
    assert(i < 3 && "not enough back buffer names");
    m_swapChainBuffersHandles[i] = TEXTURE_MANAGER->initializeFromResource(
        resource, SwapChainConstants::BACK_BUFFER_NAMES[i],
        D3D12_RESOURCE_STATE_PRESENT);
    m_swapChainBuffersDescriptors[i] =
        dx12::TEXTURE_MANAGER->getRTV(m_swapChainBuffersHandles[i]);
  }

  // Transition the resource from its initial state to be used as a depth
  // buffer.
  // freeing depth and re-creating it;
  if (m_depth != nullptr) {
    delete m_depth;
    m_depth = nullptr;
  }
  m_depth = new DepthTexture();
  m_depth->initialize(width, height);

  command->commandList->ResourceBarrier(
      1, &CD3DX12_RESOURCE_BARRIER::Transition(
             m_depth->getResource(), D3D12_RESOURCE_STATE_COMMON,
             D3D12_RESOURCE_STATE_DEPTH_WRITE));

  // Execute the resize commands.
  executeCommandList(GLOBAL_COMMAND_QUEUE, command);

  // Wait until resize is complete.
  flushCommandQueue(GLOBAL_COMMAND_QUEUE);

  // Update the viewport transform to cover the client area.
  m_screenViewport.TopLeftX = 0;
  m_screenViewport.TopLeftY = 0;
  m_screenViewport.Width = static_cast<float>(width);
  m_screenViewport.Height = static_cast<float>(height);
  m_screenViewport.MinDepth = 0.0f;
  m_screenViewport.MaxDepth = 1.0f;

  m_scissorRect = {0, 0, width, height};
  m_isInit = true;
  return true;
}
} // namespace dx12
} // namespace SirEngine
