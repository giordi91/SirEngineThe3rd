
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/d3dx12.h"

namespace SirEngine {
namespace dx12 {
bool DescriptorHeap::initialize(int size, D3D12_DESCRIPTOR_HEAP_TYPE type) {

  D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;

  // might need to extend this for more customization
  D3D12_DESCRIPTOR_HEAP_FLAGS shaderVisible =
      (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
          ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
          : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  // arbitrary magic number, need to see how to handle this
  cbvHeapDesc.NumDescriptors = size;
  cbvHeapDesc.Type = type;
  cbvHeapDesc.Flags = shaderVisible;
  cbvHeapDesc.NodeMask = 0;
  HRESULT result =
      DEVICE->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_heap));
  if (FAILED(result)) {
    return false;
  }

  m_descriptorSize = DEVICE->GetDescriptorHandleIncrementSize(type);
  m_type = type;

  // resize the freelist change
  m_freeList.resize(size);
  return true;
}
DescriptorHeap::~DescriptorHeap() {
  if (m_heap) {
    m_heap->Release();
  }
}

// Allocate a descriptor and return its index.
// If the passed descriptorIndexToUse is valid, it will be used instead of
// allocating a new one.
UINT DescriptorHeap::allocateDescriptor(
    D3D12_CPU_DESCRIPTOR_HANDLE *cpuDescriptor, UINT descriptorIndexToUse) {
  auto descriptorHeapCpuBase = getCPUStart();
  if (descriptorIndexToUse >= getDesc().NumDescriptors) {
    // we need to create a descriptor, lets check if there is any in the free
    // list
    if (m_freeListIdx != 0) {
      // get first idx
      descriptorIndexToUse = m_freeList[0];
      // patch hole
      m_freeList[0] = m_freeList[m_freeListIdx - 1];
      m_freeListIdx -= 1;
    } else {
      descriptorIndexToUse = m_descriptorsAllocated++;
    }
  }
  *cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(
      descriptorHeapCpuBase, descriptorIndexToUse, m_descriptorSize);
  return descriptorIndexToUse;
}

UINT DescriptorHeap::createBufferCBV(DescriptorPair &pair,
                                     ID3D12Resource *resource,
                                     int totalSizeInByte) {

  D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
  cbvDesc.BufferLocation = resource->GetGPUVirtualAddress();
  cbvDesc.SizeInBytes = totalSizeInByte;

  UINT descriptorIndex = allocateDescriptor(&pair.cpuHandle);
  DEVICE->CreateConstantBufferView(&cbvDesc, pair.cpuHandle);

  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(getGPUStart(), descriptorIndex,
                                                 m_descriptorSize);
#if SE_DEBUG
  pair.type = DescriptorType::CBV;
#endif
  return descriptorIndex;
}

UINT DescriptorHeap::createTexture2DSRV(DescriptorPair &pair,
                                        ID3D12Resource *resource,
                                        DXGI_FORMAT format) {
  UINT descriptorIndex = allocateDescriptor(&pair.cpuHandle);

  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Format = format;
  srvDesc.Texture2D.MipLevels = desc.MipLevels;
  srvDesc.Texture2D.MostDetailedMip = 0;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  DEVICE->CreateShaderResourceView(resource, &srvDesc, pair.cpuHandle);
  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(getGPUStart(), descriptorIndex,
                                                 m_descriptorSize);
#if SE_DEBUG
  pair.type = DescriptorType::SRV;
#endif
  return descriptorIndex;
}
UINT DescriptorHeap::createTextureCubeSRV(DescriptorPair &pair,
                                        ID3D12Resource *resource,
                                        DXGI_FORMAT format) {
  UINT descriptorIndex = allocateDescriptor(&pair.cpuHandle);

  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
  srvDesc.Format = format;
  srvDesc.TextureCube.MipLevels = desc.MipLevels;
  srvDesc.TextureCube.MostDetailedMip = 0;
  srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  DEVICE->CreateShaderResourceView(resource, &srvDesc, pair.cpuHandle);
  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(getGPUStart(), descriptorIndex,
                                                 m_descriptorSize);
#if SE_DEBUG
  pair.type = DescriptorType::SRV;
#endif
  return descriptorIndex;
}

UINT createDSV(DescriptorHeap *heap, ID3D12Resource *resource,
               DescriptorPair &pair, DXGI_FORMAT format) {

  assert(heap->getType() == D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  UINT descriptorIndex = heap->allocateDescriptor(&pair.cpuHandle);
  // Hard-code depthstencil format
  // Create descriptor to mip level 0 of entire resource using the format of the
  // resource.
  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
  dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
  dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  dsvDesc.Format = format;
  dsvDesc.Texture2D.MipSlice = 0;
  DEVICE->CreateDepthStencilView(resource, &dsvDesc, pair.cpuHandle);

  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
      heap->getGPUStart(), descriptorIndex, heap->getDescriptorSize());
#if SE_DEBUG
  pair.type = DescriptorType::DSV;
#endif
  return descriptorIndex;
}

int DescriptorHeap::reserveDescriptor(DescriptorPair &pair) {

  UINT descriptorIndex = allocateDescriptor(&pair.cpuHandle);
  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(getGPUStart(), descriptorIndex,
                                                 m_descriptorSize);
  return descriptorIndex;
}

UINT DescriptorHeap::createBufferSRV(DescriptorPair &pair,
                                     ID3D12Resource *resource, UINT numElements,
                                     UINT elementSize) {

  // SRV
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Buffer.NumElements = numElements;
  if (elementSize == 0) {

    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.StructureByteStride = 0;

  } else {
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.StructureByteStride = elementSize;
  }
  UINT descriptorIndex = allocateDescriptor(&pair.cpuHandle);
  DEVICE->CreateShaderResourceView(resource, &srvDesc, pair.cpuHandle);
  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(getGPUStart(), descriptorIndex,
                                                 m_descriptorSize);

#if SE_DEBUG
  pair.type = DescriptorType::SRV;
#endif
  return descriptorIndex;
};

UINT createRTVSRV(DescriptorHeap *heap, ID3D12Resource *resource,
                  DescriptorPair &pair) {
  assert(heap->getType() == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  UINT descriptorIndex = heap->allocateDescriptor(&pair.cpuHandle);
  DEVICE->CreateRenderTargetView(resource, nullptr, pair.cpuHandle);

  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
      heap->getGPUStart(), descriptorIndex, heap->getDescriptorSize());
#if SE_DEBUG
  pair.type = DescriptorType::RTV;
#endif

  return descriptorIndex;
}
} // namespace dx12
} // namespace SirEngine
