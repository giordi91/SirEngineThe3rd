
#include "platform/windows/graphics/dx12/texture2D.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"

#define STB_IMAGE_IMPLEMENTATION
#include "platform/windows/graphics/dx12/stb_image.h"

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

inline unsigned char *getTextureDataFromFile(const std::string &path,
                                             int *outWidth, int *outHeight) {
  unsigned char *cpu_data =
      stbi_load(path.c_str(), outWidth, outHeight, 0, STBI_rgb_alpha);
  if (cpu_data == nullptr) {
    SE_CORE_ERROR("Error loading texture: {0}", path);
    return nullptr;
  }
  return cpu_data;
}


bool Texture2D::loadFromFile(const char *path) {

  //bool res = createTextureResources(device, m_cpu_data, HDR, correctGamma);
  //assert(res == true);
  //return res;
}
} // namespace dx12
} // namespace SirEngine
