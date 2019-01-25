#include "platform/windows/graphics/dx12/constantBuffer.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <d3d12.h>

namespace temp{
namespace system{
bool ConstantBuffer::initialize(ID3D12Device *device, SirEngine::dx12::DescriptorHeap *heap,
                                int dataSizeInBytes, int offset) {
  m_size = dataSizeInBytes;
  //minimum size for constant buffer is 256 bytes, so we pad it in the 
  //case it would be smaller
  dataSizeInBytes = dataSizeInBytes % 256 == 0
                        ? dataSizeInBytes
                        : ((dataSizeInBytes / 256) + 1) * 256;
  m_actualSize = dataSizeInBytes;

  // create the upload buffer
  device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(m_actualSize),
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_buffer.resource));

  heap->createBufferCBV(&m_buffer,m_actualSize);

  bool result = m_buffer.resource != nullptr;
  return result;
}

} // namespace rendering
} // namespace dx12
