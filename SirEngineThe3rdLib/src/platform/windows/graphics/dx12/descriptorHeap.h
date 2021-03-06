#pragma once

#include "SirEngine/core.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "SirEngine/memory/cpu/resizableVector.h"

namespace SirEngine {
namespace dx12 {

class  DescriptorHeap {
 public:
  DescriptorHeap(): m_freeList(10){};
  ~DescriptorHeap();
  bool initialize(int size, D3D12_DESCRIPTOR_HEAP_TYPE type);
  inline bool initializeAsCBVSRVUAV(const int size) {
    return initialize(size, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  }

  inline bool initializeAsRtv(const int size) {
    return initialize(size, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  }

  inline bool initializeAsSampler(const int size) {
    return initialize(size, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  }

  inline bool initializeAsDsv(const int size) {
    return initialize(size, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  }
  inline D3D12_GPU_DESCRIPTOR_HANDLE getGpuStart() const
  {
    return m_heap->GetGPUDescriptorHandleForHeapStart();
  }
  inline D3D12_CPU_DESCRIPTOR_HANDLE getCpuStart() const
  {
    return m_heap->GetCPUDescriptorHandleForHeapStart();
  }
  inline size_t getHeapSize() const { return m_freeList.size(); }
  inline size_t getAllocatedDescriptorsCount() const {
    return m_descriptorsAllocated;
  }
  inline size_t getFreeHandleCount() const { return m_freeListIdx; }
  inline ID3D12DescriptorHeap **getAddressOff() { return &m_heap; }
  inline ID3D12DescriptorHeap *getResource() const { return m_heap; }
  inline D3D12_DESCRIPTOR_HEAP_DESC getDesc() const {
    return m_heap->GetDesc();
  }
  inline uint32_t getDescriptorSize() const { return m_descriptorSize; }
  inline D3D12_DESCRIPTOR_HEAP_TYPE getType() const { return m_type; }
  inline void reset() {
    // setting the allocated descriptor back to zero, so we are free to
    // re-allocate memory, descriptors don't need to be freed, we can simply
    // re-write on the memory
    m_descriptorsAllocated = 0;
  }

  inline uint32_t findCPUDescriptorIndexFromHandle(
      D3D12_CPU_DESCRIPTOR_HANDLE handle) const {
    uint32_t idx = static_cast<uint32_t>(
        (handle.ptr - m_heap->GetCPUDescriptorHandleForHeapStart().ptr) /
        m_descriptorSize);
    return idx;
  }

  // TODO fix this UINT max crap
  uint32_t allocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE *cpuDescriptor,
                              uint32_t descriptorIndexToUse = UINT_MAX);
  void freeDescriptor(const DescriptorPair &handles) {
    assert(handles.cpuHandle.ptr != 0);
    int idx = findCPUDescriptorIndexFromHandle(handles.cpuHandle);
    // freeing is just a matter of freeing up the index
    // then it will get overwritten
    assert(idx < static_cast<int>(m_freeList.size()));
    assert(idx >= 0);
    m_freeList[m_freeListIdx++] = idx;
  }

  uint32_t createBufferSRV(DescriptorPair &pair, ID3D12Resource *resource,
                           uint32_t numElements, uint32_t elementSize,
                           uint32_t elementOffset = 0,
                           bool descriptorExists = false);
  uint32_t createBufferUAV(DescriptorPair &pair, ID3D12Resource *resource,
                           uint32_t numElements, uint32_t elementSize,
                           uint32_t elementOffset, bool descriptorExists);

  uint32_t createBufferCBV(DescriptorPair &pair, ID3D12Resource *resource,
                           int totalSizeInByte, bool descriptorExists = false);

  int reserveDescriptor(DescriptorPair &pair);
  int reserveDescriptors(DescriptorPair *pair, uint32_t count);

  uint32_t createTexture2DSRV(DescriptorPair &pair, ID3D12Resource *resource,
                              DXGI_FORMAT format, uint32_t mipLevel = 0,
                              bool descriptorExists = false);
  uint32_t createTextureCubeSRV(DescriptorPair &pair, ID3D12Resource *resource,
                                DXGI_FORMAT format,
                                bool descriptorExists = false);
  uint32_t createTexture2DUAV(DescriptorPair &pair, ID3D12Resource *resource,
                              DXGI_FORMAT format, uint32_t mipLevel = 0);

 private:
  uint32_t m_descriptorsAllocated = 0;
  ID3D12DescriptorHeap *m_heap = nullptr;
  uint32_t m_descriptorSize = 0;
  D3D12_DESCRIPTOR_HEAP_TYPE m_type;

  ResizableVector<unsigned int> m_freeList;
  unsigned int m_freeListIdx = 0;
};

// this function are kept externally because refer to a particular type of
// of texture
uint32_t createRTVSRV(DescriptorHeap *heap, ID3D12Resource *resource,
                      DescriptorPair &pair);
uint32_t createDSV(DescriptorHeap *heap, ID3D12Resource *resource,
                   DescriptorPair &pair, DXGI_FORMAT format,
                   const D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
}  // namespace dx12
}  // namespace SirEngine
