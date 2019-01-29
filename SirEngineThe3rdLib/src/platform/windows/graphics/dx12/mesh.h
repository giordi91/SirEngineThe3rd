#pragma once
#include "platform/windows/graphics/dx12/DX12.h"
#include <DirectXMath.h>
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <wrl.h>

namespace SirEngine {
namespace dx12 {

struct Mesh {

public:
  Mesh() = default;
  ~Mesh();

  void loadFromFile(ID3D12Device *device, const std::string &path,
                    DescriptorHeap *heap);

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
    ibv.Format = DXGI_FORMAT_R32_UINT;
    ibv.SizeInBytes = m_indexCount * sizeof(int);
    return ibv;
  }

  void translate(float x, float y, float z);

  inline DirectX::XMMATRIX getMatrix() const { return transform; }

  inline UINT getIndexCount() const { return m_indexCount; }
  inline UINT getIndexSize() const { return m_indexSize; }
  inline UINT getVertexCount() const { return m_vertexCount; }
  inline UINT getVertexSize() const { return m_vertexSize; }
  inline UINT getStride() const { return m_stride; }
  inline const std::vector<float> &getCpuVertex() const { return m_vertexs; };

public:
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
