
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
uint32_t DescriptorHeap::allocateDescriptor(
    D3D12_CPU_DESCRIPTOR_HANDLE *cpuDescriptor, uint32_t descriptorIndexToUse) {
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

uint32_t DescriptorHeap::createBufferCBV(DescriptorPair &pair, ID3D12Resource *resource,
                           int totalSizeInByte,bool descriptorExists) {
  D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
  cbvDesc.BufferLocation = resource->GetGPUVirtualAddress();
  cbvDesc.SizeInBytes = totalSizeInByte;

  uint32_t descriptorIndex = 0;
  if (!descriptorExists) {
    descriptorIndex = allocateDescriptor(&pair.cpuHandle);
  } else {
    // reconstruct the descriptor index using pointers
    auto descriptorHeapCpuBase = getCPUStart();
    descriptorIndex =
        (pair.cpuHandle.ptr - descriptorHeapCpuBase.ptr) / m_descriptorSize;
  }

  DEVICE->CreateConstantBufferView(&cbvDesc, pair.cpuHandle);

  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(getGPUStart(), descriptorIndex,
                                                 m_descriptorSize);
#if SE_DEBUG
  pair.type = DescriptorType::CBV;
#endif
  return descriptorIndex;
}

uint32_t DescriptorHeap::createTexture2DSRV(DescriptorPair &pair,
                                            ID3D12Resource *resource,
                                            DXGI_FORMAT format,
                                            uint32_t mipLevel,
                                            bool descriptorExists) {
  uint32_t descriptorIndex = 0;
  if (!descriptorExists) {
    descriptorIndex = allocateDescriptor(&pair.cpuHandle);
  } else {
    // reconstruct the descriptor index using pointers
    auto descriptorHeapCpuBase = getCPUStart();
    descriptorIndex =
        (pair.cpuHandle.ptr - descriptorHeapCpuBase.ptr) / m_descriptorSize;
  }

  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Format = format;
  srvDesc.Texture2D.MipLevels = desc.MipLevels;
  srvDesc.Texture2D.MostDetailedMip = mipLevel;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  DEVICE->CreateShaderResourceView(resource, &srvDesc, pair.cpuHandle);
  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(getGPUStart(), descriptorIndex,
                                                 m_descriptorSize);
#if SE_DEBUG
  pair.type = DescriptorType::SRV;
#endif
  return descriptorIndex;
}

uint32_t DescriptorHeap::createTexture2DUAV(DescriptorPair &pair,
                                            ID3D12Resource *resource,
                                            DXGI_FORMAT format,
                                            uint32_t mipLevel) {
  uint32_t descriptorIndex = allocateDescriptor(&pair.cpuHandle);

  D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
  UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  UAVDesc.Format = format;
  UAVDesc.Texture2D.MipSlice = mipLevel;
  DEVICE->CreateUnorderedAccessView(resource, nullptr, &UAVDesc,
                                    pair.cpuHandle);
  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(getGPUStart(), descriptorIndex,
                                                 m_descriptorSize);
#if SE_DEBUG
  pair.type = DescriptorType::UAV;
#endif
  return descriptorIndex;
}

uint32_t DescriptorHeap::createTextureCubeSRV(DescriptorPair &pair,
                                              ID3D12Resource *resource,
                                              DXGI_FORMAT format, bool descriptorExists) {
  uint32_t descriptorIndex;

  if (!descriptorExists) {
    descriptorIndex = allocateDescriptor(&pair.cpuHandle);
  } else {
    // reconstruct the descriptor index using pointers
    auto descriptorHeapCpuBase = getCPUStart();
    descriptorIndex =
        (pair.cpuHandle.ptr - descriptorHeapCpuBase.ptr) / m_descriptorSize;
  }

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

uint32_t createDSV(DescriptorHeap *heap, ID3D12Resource *resource,
                   DescriptorPair &pair, const DXGI_FORMAT format,
                   const D3D12_DSV_FLAGS flags) {
  assert(heap->getType() == D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  uint32_t descriptorIndex = heap->allocateDescriptor(&pair.cpuHandle);
  // Hard-code depth-stencil format
  // Create descriptor to mip level 0 of entire resource using the format of the
  // resource.
  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
  dsvDesc.Flags = flags;
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
  uint32_t descriptorIndex = allocateDescriptor(&pair.cpuHandle);
  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(getGPUStart(), descriptorIndex,
                                                 m_descriptorSize);
  return descriptorIndex;
}

int DescriptorHeap::reserveDescriptors(DescriptorPair *pair,
                                       const uint32_t count) {
  assert(count > 0 && "asking for zero reserved descriptors");
  uint32_t baseDescriptorIndex = 0;
  for (uint32_t i = 0; i < count; ++i) {
    uint32_t descriptorIndex = allocateDescriptor(&pair[i].cpuHandle);
    pair[i].gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        getGPUStart(), descriptorIndex, m_descriptorSize);
    baseDescriptorIndex = i == 0 ? descriptorIndex : baseDescriptorIndex;
  }
  return baseDescriptorIndex;
}

uint32_t DescriptorHeap::createBufferSRV(
    DescriptorPair &pair, ID3D12Resource *resource, uint32_t numElements,
    uint32_t elementSize, uint32_t elementOffset, bool descriptorExists) {
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
    srvDesc.Buffer.FirstElement = elementOffset;

  } else {
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.StructureByteStride = elementSize;
    srvDesc.Buffer.FirstElement = elementOffset;
  }
  uint32_t descriptorIndex;
  if (!descriptorExists) {
    descriptorIndex = allocateDescriptor(&pair.cpuHandle);
  } else {
    // reconstruct the descriptor index using pointers
    auto descriptorHeapCpuBase = getCPUStart();
    descriptorIndex =
        (pair.cpuHandle.ptr - descriptorHeapCpuBase.ptr) / m_descriptorSize;
  }

  DEVICE->CreateShaderResourceView(resource, &srvDesc, pair.cpuHandle);
  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(getGPUStart(), descriptorIndex,
                                                 m_descriptorSize);

#if SE_DEBUG
  pair.type = DescriptorType::SRV;
#endif
  return descriptorIndex;
};

uint32_t DescriptorHeap::createBufferUAV(DescriptorPair &pair,
                                         ID3D12Resource *resource,
                                         uint32_t numElements,
                                         uint32_t elementSize) {
  // SRV
  D3D12_UNORDERED_ACCESS_VIEW_DESC srvDesc = {};
  srvDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  srvDesc.Buffer.FirstElement = 0;
  srvDesc.Buffer.NumElements = numElements;
  srvDesc.Buffer.StructureByteStride = elementSize;

  // srvDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

  uint32_t descriptorIndex = allocateDescriptor(&pair.cpuHandle);
  DEVICE->CreateUnorderedAccessView(resource, nullptr, &srvDesc,
                                    pair.cpuHandle);
  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(getGPUStart(), descriptorIndex,
                                                 m_descriptorSize);

#if SE_DEBUG
  pair.type = DescriptorType::UAV;
#endif
  return descriptorIndex;
};

uint32_t createRTVSRV(DescriptorHeap *heap, ID3D12Resource *resource,
                      DescriptorPair &pair) {
  assert(heap->getType() == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  uint32_t descriptorIndex = heap->allocateDescriptor(&pair.cpuHandle);
  DEVICE->CreateRenderTargetView(resource, nullptr, pair.cpuHandle);

  pair.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
      heap->getGPUStart(), descriptorIndex, heap->getDescriptorSize());
#if SE_DEBUG
  pair.type = DescriptorType::RTV;
#endif

  return descriptorIndex;
}
}  // namespace dx12
}  // namespace SirEngine
