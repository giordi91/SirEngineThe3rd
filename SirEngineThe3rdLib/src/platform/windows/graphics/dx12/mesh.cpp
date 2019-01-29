#include "platform/windows/graphics/dx12/mesh.h"
#include "SirEngine/binary/binaryFile.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <wrl.h>

namespace SirEngine {
namespace dx12 {

inline void AllocateUploadBuffer(ID3D12Device *pDevice, void *pData,
                                 UINT64 datasize, ID3D12Resource **ppResource,
                                 const wchar_t *resourceName = nullptr) {
  auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
  pDevice->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(ppResource));

  if (resourceName) {
    (*ppResource)->SetName(resourceName);
  }
  void *pMappedData;
  (*ppResource)->Map(0, nullptr, &pMappedData);
  memcpy(pMappedData, pData, datasize);
  (*ppResource)->Unmap(0, nullptr);
}

Microsoft::WRL::ComPtr<ID3D12Resource>
createDefaultBuffer(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList,
                    const void *initData, UINT64 byteSize,
                    Microsoft::WRL::ComPtr<ID3D12Resource> &uploadBuffer) {
  Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;

  // Create the actual default buffer resource.
  HRESULT res = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(byteSize), D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(defaultBuffer.GetAddressOf()));

  // In order to copy CPU memory data into our default buffer, we need to create
  // an intermediate upload heap.
  HRESULT res2 = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

  // Describe the data we want to copy into the default buffer.
  D3D12_SUBRESOURCE_DATA subResourceData = {};
  subResourceData.pData = initData;
  subResourceData.RowPitch = byteSize;
  subResourceData.SlicePitch = subResourceData.RowPitch;

  // Schedule to copy the data to the default buffer resource.  At a high level,
  // the helper function UpdateSubresources will copy the CPU memory into the
  // intermediate upload heap.  Then, using
  // ID3D12CommandList::CopySubresourceRegion, the intermediate upload heap data
  // will be copied to mBuffer.
  cmdList->ResourceBarrier(
      1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
                                               D3D12_RESOURCE_STATE_COMMON,
                                               D3D12_RESOURCE_STATE_COPY_DEST));
  UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0,
                        1, &subResourceData);
  cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                  defaultBuffer.Get(),
                                  D3D12_RESOURCE_STATE_COPY_DEST,
                                  D3D12_RESOURCE_STATE_GENERIC_READ));

  // Note: uploadBuffer has to be kept alive after the above function calls
  // because the command list has not been executed yet that performs the actual
  // copy. The caller can Release the uploadBuffer after it knows the copy has
  // been executed.

  return defaultBuffer;
}

Mesh::~Mesh()
{

	m_uploadIndex.Reset();
	m_uploadVertex.Reset();
	m_defaultIndex.Reset();
	m_defaultVertex.Reset();
}

void Mesh::loadFromFile(ID3D12Device *device,
                                      const std::string &path,
                                      DescriptorHeap *heap) {

  std::vector<char> data;
  readAllBytes(path, data);
  const BinaryFileHeader *h = getHeader(data.data());


  auto mapper = getMapperData<ModelMapperData>(data.data());

  m_stride = mapper->strideInByte / sizeof(float);
  // creating the buffers
  int sz = mapper->vertexDataSizeInByte / mapper->strideInByte;
  render_index_size = mapper->indexDataSizeInByte / sizeof(int);

  // lets get the vertex data
  float *vertexData = reinterpret_cast<float *>(data.data() + sizeof(BinaryFileHeader));
  int*indexData = reinterpret_cast<int*>(data.data() + sizeof(BinaryFileHeader) +
	  mapper->vertexDataSizeInByte);

  auto* currentFC = &CURRENT_FRAME_RESOURCE->fc;
  m_indexCount = render_index_size;
  m_defaultVertex = createDefaultBuffer(
      DEVICE, currentFC->commandList, vertexData,
      sz * m_stride * sizeof(float), m_uploadVertex);
  m_defaultIndex = createDefaultBuffer(
      DEVICE, currentFC->commandList, indexData,
      render_index_size * sizeof(int), m_uploadIndex);
  
  m_bufferVS.resource = m_defaultVertex.Get();
  m_bufferIDX.resource = m_defaultIndex.Get();

  heap->createBufferSRV(
      &(m_bufferIDX),
      getIndexCount() 
           // getIndexCount return the numeber of 16 bit elements,
            // srv expect the 32 bit number of elements, so dividing by 2
      ,
      sizeof(int));
  m_vertexCount = sz;
  m_vertexSize = m_stride;
  heap->createBufferSRV(&(m_bufferVS), getVertexCount(), getVertexSize());
}


void Mesh::translate(float x, float y, float z) {
  DirectX::XMFLOAT4X4 meshTView{};
  XMStoreFloat4x4(&meshTView, transform);
  meshTView._41 = x;
  meshTView._42 = y;
  meshTView._43 = z;
  transform = XMLoadFloat4x4(&meshTView);
}
} // namespace dx12
} // namespace SirEngine
