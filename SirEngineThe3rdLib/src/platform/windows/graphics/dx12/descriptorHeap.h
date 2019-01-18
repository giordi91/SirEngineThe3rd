#pragma once
#include "platform/windows/graphics/dx12/DX12.h"
#include <vector>

namespace SirEngine {
namespace dx12 {

class DescriptorHeap {

public:
  DescriptorHeap() = default;
  ~DescriptorHeap();
  bool initialize(int size, D3D12_DESCRIPTOR_HEAP_TYPE type);
  inline bool initializeAsCBVSRVUAV(int size) {
    return initialize(size, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  }

  inline bool initializeAsRTV(int size) {
    return initialize(size, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  }

  inline bool initializeAsDSV(int size) {
    return initialize(size, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  }
  inline D3D12_GPU_DESCRIPTOR_HANDLE getGPUStart() {
    return m_heap->GetGPUDescriptorHandleForHeapStart();
  }
  inline D3D12_CPU_DESCRIPTOR_HANDLE getCPUStart() {
    return m_heap->GetCPUDescriptorHandleForHeapStart();
  }
  inline ID3D12DescriptorHeap **getAddressOff() { return &m_heap; }
  inline ID3D12DescriptorHeap *getResource() { return m_heap; }
  inline D3D12_DESCRIPTOR_HEAP_DESC getDesc() { return m_heap->GetDesc(); }
  inline UINT getDescriptorSize() { return m_descriptorSize; }
  inline D3D12_DESCRIPTOR_HEAP_TYPE getType() { return m_type; }
  inline void reset() {
    // setting the allocated descriptor back to zero, so we are free to
    // re-allocate memory, descriptors don't need to be freed, we can simply
    // re-write on the memory
    m_descriptorsAllocated = 0;
  }

  inline UINT
  findCPUDescriptorIndexFromHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    int idx = (handle.ptr - m_heap->GetCPUDescriptorHandleForHeapStart().ptr) /
              m_descriptorSize;
    return idx;
  }

  UINT allocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE *cpuDescriptor,
                          UINT descriptorIndexToUse = UINT_MAX);
  void freeDescritpor(D3DBuffer &handles) {
    assert(handles.cpuDescriptorHandle.ptr != 0);
    int idx = findCPUDescriptorIndexFromHandle(handles.cpuDescriptorHandle);
    // freeing is just a matter of freeing up the index
    // then it will get overritten
    m_freeList[m_freeListIdx++] = idx;
  }

  UINT createBufferSRV(D3DBuffer *buffer, UINT numElements, UINT elementSize);

  UINT createBufferCBV(D3DBuffer *buffer, int totalSizeInByte);

  int reserveDescriptor(D3DBuffer *buffer);

  UINT createTexture2DUAV(D3DBuffer *buffer, DXGI_FORMAT format);
  UINT createTexture2DSRV(D3DBuffer *buffer, DXGI_FORMAT format);
  // UINT createDSV(system::D3DBuffer *buffer);

private:
  UINT m_descriptorsAllocated = 0;
  ID3D12DescriptorHeap *m_heap = nullptr;
  UINT m_descriptorSize = 0;
  D3D12_DESCRIPTOR_HEAP_TYPE m_type;

  //
  std::vector<unsigned int> m_freeList;
  unsigned int m_freeListIdx = 0;
};

// this function are kept externally because refer to a particular type of
// of texture
UINT createRTVSRV(DescriptorHeap *heap, D3DBuffer *buffer);
UINT createDSV(DescriptorHeap *heap, D3DBuffer *buffer);
} // namespace dx12
} // namespace SirEngine
