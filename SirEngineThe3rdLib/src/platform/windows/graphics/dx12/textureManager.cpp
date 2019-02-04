#include "platform/windows/graphics/dx12/textureManager.h"

#include "SirEngine/fileUtils.h"
#include <DXTK12/DDSTextureLoader.h>

namespace SirEngine {
namespace dx12 {
TextureManager::~TextureManager()
{
	assert(m_texturePool.assertEverythingDealloc());
}

TextureHandle TextureManager::loadTexture(const char *path, bool dynamic) {
  bool res = fileExists(path);

  const std::string name = getFileName(path);
  assert(m_nameToHandle.find(name) == m_nameToHandle.end());

  assert(res);

  if (dynamic) {
    assert(0 && "dynamic textures are not yet implemented");
  }

  uint32_t index;
  TextureData &data = m_texturePool.getFreeMemoryData(index);
  const std::string paths(path);
  const std::wstring pathws(paths.begin(), paths.end());
  std::unique_ptr<uint8_t[]> ddsData;
  std::vector<D3D12_SUBRESOURCE_DATA> subresources;

  batch.Begin();
  DirectX::CreateDDSTextureFromFile(dx12::DEVICE, batch, pathws.c_str(),
                                    &data.resource, false);
  batch.End(dx12::GLOBAL_COMMAND_QUEUE);

  // data is now loaded need to create handle etc
  TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.format = data.resource->GetDesc().Format;
  data.state = D3D12_RESOURCE_STATE_COMMON;
  // m_staticStorage.push_back(data);

  ++MAGIC_NUMBER_COUNTER;

  m_nameToHandle[name] = handle;
  return handle;
}

TextureHandle TextureManager::initializeFromResource(
    ID3D12Resource *resource, const char *name, D3D12_RESOURCE_STATES state) {
  // since we are passing one resource, by definition the resource is static
  // data is now loaded need to create handle etc
  uint32_t index;
  TextureData &data = m_texturePool.getFreeMemoryData(index);
  TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

  data.resource = resource;
  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.format = data.resource->GetDesc().Format;
  data.state = state;

  ++MAGIC_NUMBER_COUNTER;

  m_nameToHandle[name] = handle;
  return handle;
}

TextureHandle TextureManager::createDepthTexture(const char *name,
                                                 uint32_t width,
                                                 uint32_t height,
                                                 D3D12_RESOURCE_STATES state) {
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
  HRESULT res =
      DEVICE->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                  &msQualityLevels, sizeof(msQualityLevels));
  assert(SUCCEEDED(res));
  UINT m_msaaQuality = msQualityLevels.NumQualityLevels;

  depthStencilDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
  depthStencilDesc.SampleDesc.Quality = m_4xMsaaState ? (m_msaaQuality - 1) : 0;
  depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  uint32_t index;
  TextureData &data = m_texturePool.getFreeMemoryData(index);
  D3D12_CLEAR_VALUE optClear;
  optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  optClear.DepthStencil.Depth = 1.0f;
  optClear.DepthStencil.Stencil = 0;
  res = DEVICE->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
      &depthStencilDesc, state, &optClear, IID_PPV_ARGS(&data.resource));
  assert(SUCCEEDED(res));

  data.flags = DebugTextureFlags::DEPTH;
  // we have the texture, we set the flag, we now need to create handle etc
  // data is now loaded need to create handle etc
  TextureHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};

  data.magicNumber = MAGIC_NUMBER_COUNTER;
  data.format = data.resource->GetDesc().Format;
  data.state = state;

  ++MAGIC_NUMBER_COUNTER;

  m_nameToHandle[name] = handle;
  return handle;

  // createDSV(GLOBAL_DSV_HEAP, &m_texture);
}
} // namespace dx12
} // namespace SirEngine
