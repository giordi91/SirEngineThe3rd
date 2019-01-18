
#include "platform/windows/graphics/dx12/texture2D.h"
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
    dx12::DX12Handles::globalCBVSRVUAVheap->freeDescritpor(buffer);
    break;
  }
  case (DescriptorType::RTV): {
    dx12::DX12Handles::globalRTVheap->freeDescritpor(buffer);
    break;
  }
  case (DescriptorType::DSV): {
    dx12::DX12Handles::globalDSVheap->freeDescritpor(buffer);
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
  HRESULT hr = DX12Handles::device->CreateCommittedResource(
      &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc,
      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
      IID_PPV_ARGS(&m_texture.resource));
  if (FAILED(hr)) {
    return false;
  }

  // NAME_D3D12_OBJECT(m_texture.resource);
  DX12Handles::globalCBVSRVUAVheap->createTexture2DUAV(&m_texture, format);
  DX12Handles::globalCBVSRVUAVheap->createTexture2DSRV(&m_textureSRV, format);

  return true;
}

bool Texture2D::initializeEmptyRT(int width, int height, DXGI_FORMAT format) {

  // Create the output resource. The dimensions and format should match the
  // swap-chain.
  auto uavDesc =
      CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, 1, 0,
                                   D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

  auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  HRESULT hr = DX12Handles::device->CreateCommittedResource(
      &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc,
      D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr,
      IID_PPV_ARGS(&m_texture.resource));
  if (FAILED(hr)) {
    return false;
  }

  createRTVSRV(DX12Handles::globalRTVheap, &m_texture);
  m_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

  return true;
}

bool Texture2D::initializeFromResource(ID3D12Resource *resource,
                                       DXGI_FORMAT format) {

  m_texture.resource = resource;
  auto desc = resource->GetDesc();

  // NAME_D3D12_OBJECT(m_texture.resource);
  DX12Handles::globalCBVSRVUAVheap->createTexture2DUAV(&m_texture, format);

  return true;
}
bool Texture2D::initializeRTFromResource(ID3D12Resource *resource) {

  m_texture.resource = resource;
  createRTVSRV(DX12Handles::globalRTVheap, &m_texture);
  m_currentState = D3D12_RESOURCE_STATE_PRESENT;

  return true;
}
void Texture2D::clear() {
  if (m_texture.resource != nullptr) {
    m_texture.resource->Release();
  }
}
} // namespace dx12
} // namespace SirEngine
