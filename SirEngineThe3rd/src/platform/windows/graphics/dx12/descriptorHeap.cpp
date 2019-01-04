#include "SirEnginepch.h"

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
  HRESULT result = DX12Handles::device->CreateDescriptorHeap(
      &cbvHeapDesc, IID_PPV_ARGS(&m_heap));
  if (FAILED(result)) {
    return false;
  }

  m_descriptorSize =
      DX12Handles::device->GetDescriptorHandleIncrementSize(type);
  m_type = type;
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
    descriptorIndexToUse = m_descriptorsAllocated++;
  }
  *cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(
      descriptorHeapCpuBase, descriptorIndexToUse, m_descriptorSize);
  return descriptorIndexToUse;
}

UINT DescriptorHeap::createBufferCBV(D3DBuffer *buffer,
                                     int totalSizeInByte) {

  D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
  cbvDesc.BufferLocation = buffer->resource->GetGPUVirtualAddress();
  cbvDesc.SizeInBytes = totalSizeInByte;

  UINT descriptorIndex = allocateDescriptor(&buffer->cpuDescriptorHandle);
  DX12Handles::device->CreateConstantBufferView(&cbvDesc, buffer->cpuDescriptorHandle);

  buffer->gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
      getGPUStart(), descriptorIndex, m_descriptorSize);
  return descriptorIndex;
}

UINT DescriptorHeap::createTexture2DUAV(D3DBuffer *buffer,
                                        DXGI_FORMAT format) {
  UINT descriptorIndex = allocateDescriptor(&buffer->cpuDescriptorHandle);

  D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
  UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  UAVDesc.Format = format;
  DX12Handles::device->CreateUnorderedAccessView(buffer->resource, nullptr, &UAVDesc,
                                      buffer->cpuDescriptorHandle);
  buffer->gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
      getGPUStart(), descriptorIndex, m_descriptorSize);
  return descriptorIndex;
}

UINT DescriptorHeap::createTexture2DSRV(D3DBuffer *buffer,
                                        DXGI_FORMAT format) {
  UINT descriptorIndex = allocateDescriptor(&buffer->cpuDescriptorHandle);

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Format = format;
  srvDesc.Texture2D.MipLevels = 1;
  srvDesc.Texture2D.MostDetailedMip = 0;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  DX12Handles::device->CreateShaderResourceView(buffer->resource, &srvDesc,
                                     buffer->cpuDescriptorHandle);
  buffer->gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
      getGPUStart(), descriptorIndex, m_descriptorSize);
  return descriptorIndex;
}

UINT createDSV(DescriptorHeap *heap, D3DBuffer *buffer) {

  assert(heap->getType() == D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  UINT descriptorIndex = heap->allocateDescriptor(&buffer->cpuDescriptorHandle);
  // Hard-code depthstencil format
  // Create descriptor to mip level 0 of entire resource using the format of the
  // resource.

  const DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
  dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
  dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  dsvDesc.Format = m_depthStencilFormat;
  dsvDesc.Texture2D.MipSlice = 0;
  DX12Handles::device->CreateDepthStencilView(buffer->resource, &dsvDesc,
                                 buffer->cpuDescriptorHandle);

  buffer->gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
      heap->getGPUStart(), descriptorIndex, heap->getDescriptorSize());
  return descriptorIndex;
}

UINT DescriptorHeap::createBufferSRV(D3DBuffer *buffer,
                                     UINT numElements, UINT elementSize) {

  // SRV
  auto s = buffer->resource->GetGPUVirtualAddress();
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Buffer.NumElements = numElements;
  if (elementSize == 0) {

    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    srvDesc.Buffer.StructureByteStride = 0;

  } else {
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.StructureByteStride = elementSize;
  }
  UINT descriptorIndex = allocateDescriptor(&buffer->cpuDescriptorHandle);
  DX12Handles::device->CreateShaderResourceView(buffer->resource, &srvDesc,
                                     buffer->cpuDescriptorHandle);
  buffer->gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
      getGPUStart(), descriptorIndex, m_descriptorSize);
  return descriptorIndex;
};

UINT createRTVSRV(DescriptorHeap *heap, D3DBuffer *buffer) {

  assert(heap->getType() == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  UINT descriptorIndex = heap->allocateDescriptor(&buffer->cpuDescriptorHandle);
  DX12Handles::device->CreateRenderTargetView(buffer->resource, nullptr,
                                 buffer->cpuDescriptorHandle);

  buffer->gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
      heap->getGPUStart(), descriptorIndex, heap->getDescriptorSize());
  return descriptorIndex;
}
} // namespace dx12
} // namespace SirEngine
