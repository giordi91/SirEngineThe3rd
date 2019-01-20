#pragma once
#include "platform/windows/graphics/dx12/DX12.h"
#include <DirectXMath.h>
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <wrl.h>

namespace tinyobj {
struct attrib_t;
struct shape_t;
} // namespace tinyobj

namespace SirEngine {
namespace dx12 {

struct Dx12RaytracingMesh {

public:
  Dx12RaytracingMesh() = default;

  // void load(ID3D12Device *device, tinyobj::attrib_t &attr,
  //          tinyobj::shape_t &shape);
  void loadFromFile(ID3D12Device *device, const std::string &path,
                    DescriptorHeap *heap);
  // void loadExternal(ID3D12Device *device, ID3D12Resource *idxData,
  //                  ID3D12Resource *vsData, int stride, int idxCount,
  //                  int vtxCount, DescriptorHeap *heap);

  // void loadExternalCPU(ID3D12Device *device, uint16_t *idxData, float
  // *vsData,
  //                     int stride, int idxCount, int vtxCount,
  //                     DescriptorHeap *heap);
  inline D3D12_VERTEX_BUFFER_VIEW getVertexBufferView() const {
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = m_bufferVS.resource->GetGPUVirtualAddress();
    vbv.StrideInBytes = m_stride * sizeof(float);
    vbv.SizeInBytes = vbv.StrideInBytes * (m_vertexCount);
    return vbv;
  }

  inline D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const {
    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = m_bufferIDX.resource->GetGPUVirtualAddress();
    ibv.Format = DXGI_FORMAT_R16_UINT;
    ibv.SizeInBytes = m_indexCount * sizeof(uint16_t);
    return ibv;
  }

  void translate(float x, float y, float z);

  inline DirectX::XMMATRIX getMatrix() { return transform; }

  inline UINT getIndexCount() { return m_indexCount; }
  inline UINT getIndexSize() { return m_indexSize; }
  inline UINT getVertexCount() { return m_vertexCount; }
  inline UINT getVertexSize() { return m_vertexSize; }
  inline UINT getStride() { return m_stride; }
  inline const std::vector<float> &getCPUVertex() { return m_vertexs; };

public:
  ID3D12Resource *idxdata;
  ID3D12Resource *vsdata;
  D3DBuffer m_bufferIDX;
  D3DBuffer m_bufferVS;

private:
  std::vector<float> m_vertexs;
  std::vector<uint16_t> render_index;
  unsigned int render_index_size;
  DirectX::XMMATRIX transform = DirectX::XMMatrixIdentity();
  int m_stride;
  std::vector<unsigned int> m_cpu_vtx_data;
  UINT m_indexCount;
  UINT m_indexSize;
  UINT m_vertexCount;
  UINT m_vertexSize;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_defaultVertex;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_defaultIndex;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadVertex;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadIndex;
};
} // namespace dx12
} // namespace SirEngine
