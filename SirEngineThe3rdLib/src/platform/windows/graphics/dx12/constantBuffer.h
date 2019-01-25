#pragma once
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include <cassert>

namespace temp{
namespace system {
struct ConstantBuffer {
public:
  ConstantBuffer() = default;
  bool initialize(ID3D12Device *device, SirEngine::dx12::DescriptorHeap *heap,
                  int dataSyzeInByte, int offset);
  ~ConstantBuffer() { clear(); };
  inline ID3D12Resource *getBuffer() const { return m_buffer.resource; }
  inline D3D12_CPU_DESCRIPTOR_HANDLE getCPUView() const {
    return m_buffer.cpuDescriptorHandle;
  }
  inline D3D12_GPU_DESCRIPTOR_HANDLE getGPUView() const {
    return m_buffer.gpuDescriptorHandle;
  }
  inline void clear() {
    if (m_isMapped) {
      unmap();
    }
    if (m_buffer.resource != nullptr) {
      m_buffer.resource->Release();
    }
  }
  inline void map() {
    // assert(!m_isMapped);
    if (!m_isMapped) {
      m_isMapped = true;
      m_buffer.resource->Map(0, nullptr,
                             reinterpret_cast<void **>(&mappedData));
    }
  };
  inline void unmap() {
    assert(m_isMapped);
    m_isMapped = false;
    m_buffer.resource->Unmap(0, nullptr);
  };
  inline void update(void *data) {
    assert(mappedData != nullptr);
    memcpy(mappedData, data, m_size);
  };

  inline UINT getActualSize() const { return m_actualSize; }

private:
  ConstantBuffer(const ConstantBuffer &) = delete;
  ConstantBuffer &operator=(const ConstantBuffer &) = delete;

private:
  SirEngine::dx12::D3DBuffer m_buffer;
  int m_actualSize;
  int m_size;
  BYTE *mappedData = nullptr;
  bool m_isMapped = false;
};
} // namespace system
} // namespace dx12
