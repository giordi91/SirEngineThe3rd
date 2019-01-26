#include "platform/windows/graphics/dx12/depthTexture.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/d3dx12.h"

namespace SirEngine {
namespace dx12 {
DepthTexture::~DepthTexture() {
	dx12::GLOBAL_DSV_HEAP->freeDescritpor(m_texture);
}
bool DepthTexture::initialize(int width, int height) {

  bool m_4xMsaaState = false;

  // Create the depth/stencil buffer and view.
  D3D12_RESOURCE_DESC depthStencilDesc;
  depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  depthStencilDesc.Alignment = 0;
  depthStencilDesc.Width = width;
  depthStencilDesc.Height = height;
  depthStencilDesc.DepthOrArraySize = 1;
  depthStencilDesc.MipLevels = 1;

  // Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to
  // read from the depth buffer.  Therefore, because we need to create two views
  // to the same resource:
  //   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
  //   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
  // we need to create the depth buffer resource with a typeless format.
  depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

  // Check 4X MSAA quality support for our back buffer format.
  // All Direct3D 11 capable devices support 4X MSAA for all render
  // target formats, so we only need to check quality support.
  D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
  msQualityLevels.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  msQualityLevels.SampleCount = 4;
  msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
  msQualityLevels.NumQualityLevels = 0;
  HRESULT result = DEVICE->CheckFeatureSupport(
      D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels,
      sizeof(msQualityLevels));
  UINT m_msaaQuality = msQualityLevels.NumQualityLevels;

  depthStencilDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
  depthStencilDesc.SampleDesc.Quality = m_4xMsaaState ? (m_msaaQuality - 1) : 0;
  depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_CLEAR_VALUE optClear;
  optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  optClear.DepthStencil.Depth = 1.0f;
  optClear.DepthStencil.Stencil = 0;
  HRESULT res = DEVICE->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
      &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear,
      IID_PPV_ARGS(&m_texture.resource));

  createDSV(GLOBAL_DSV_HEAP, &m_texture);

  m_currentState = D3D12_RESOURCE_STATE_COMMON;
  return false;
}
void DepthTexture::clear() {}
} // namespace dx12
} // namespace SirEngine
