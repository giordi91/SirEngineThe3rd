
#include "platform/windows/graphics/dx12/texture2D.h"
#include "DXTK12/DDSTextureLoader.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"

namespace SirEngine {
namespace dx12 {

inline void freeTextureDescriptor(D3DBuffer &buffer) {

  if (buffer.descriptorType == DescriptorType::NONE) {
    return;
  }
  switch (buffer.descriptorType) {
  case (DescriptorType::CBV):
  case (DescriptorType::SRV):
  case (DescriptorType::UAV): {
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescritpor(buffer);
    break;
  }
  case (DescriptorType::RTV): {
    dx12::GLOBAL_RTV_HEAP->freeDescritpor(buffer);
    break;
  }
  case (DescriptorType::DSV): {
    dx12::GLOBAL_DSV_HEAP->freeDescritpor(buffer);
    break;
  }
  }
}

Texture2D::~Texture2D() {
  clear();

  // free the descriptor
  freeTextureDescriptor(m_texture);
  freeTextureDescriptor(m_textureSRV);
}
bool Texture2D::initializeEmpty(int width, int height, DXGI_FORMAT format) {

  // Create the output resource. The dimensions and format should match the
  // swap-chain.
  auto uavDesc =
      CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, 1, 0,
                                   D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

  auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  HRESULT hr = DEVICE->CreateCommittedResource(
      &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc,
      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
      IID_PPV_ARGS(&m_texture.resource));
  if (FAILED(hr)) {
    return false;
  }

  // NAME_D3D12_OBJECT(m_texture.resource);
  GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DUAV(&m_texture, format);
  GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(&m_textureSRV, format);

  return true;
}

bool Texture2D::initializeEmptyRT(int width, int height, DXGI_FORMAT format) {

  // Create the output resource. The dimensions and format should match the
  // swap-chain.
  auto uavDesc =
      CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, 1, 0,
                                   D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

  auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  HRESULT hr = DEVICE->CreateCommittedResource(
      &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc,
      D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr,
      IID_PPV_ARGS(&m_texture.resource));
  if (FAILED(hr)) {
    return false;
  }

  createRTVSRV(GLOBAL_RTV_HEAP, &m_texture);
  m_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

  return true;
}

bool Texture2D::initializeFromResource(ID3D12Resource *resource,
                                       DXGI_FORMAT format) {

  m_texture.resource = resource;
  auto desc = resource->GetDesc();

  // NAME_D3D12_OBJECT(m_texture.resource);
  GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DUAV(&m_texture, format);

  return true;
}
bool Texture2D::initializeRTFromResource(ID3D12Resource *resource) {

  m_texture.resource = resource;
  createRTVSRV(GLOBAL_RTV_HEAP, &m_texture);
  m_currentState = D3D12_RESOURCE_STATE_PRESENT;

  return true;
}
void Texture2D::clear() {
  if (m_texture.resource != nullptr) {
    m_texture.resource->Release();
    m_texture.resource = nullptr;
  }
}

bool Texture2D::loadFromFile(const char *path) {

  ID3D12Resource *resource;
  const std::string paths(path);
  const std::wstring pathws(paths.begin(),paths.end());
  std::unique_ptr<uint8_t[]> ddsData;
  std::vector<D3D12_SUBRESOURCE_DATA> subresources;
  DirectX::LoadDDSTextureFromFile(dx12::DEVICE, pathws.c_str(), &resource, ddsData,
                                  subresources);
  //DirectX::CreateDDSTextureFromFile();
  D3D12_RESOURCE_DESC d = resource->GetDesc();
  m_texture.resource = resource;
  //NAME_D3D12_OBJECT(m_texture.resource);
  GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(&m_texture, d.Format);
  return false;
  // bool res = createTextureResources(device, m_cpu_data, HDR, correctGamma);
  // assert(res == true);
  // return res;
}
} // namespace dx12
} // namespace SirEngine
