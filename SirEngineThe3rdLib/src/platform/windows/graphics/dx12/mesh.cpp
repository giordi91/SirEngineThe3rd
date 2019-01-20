#include "platform/windows/graphics/dx12/mesh.h"
#include "SirEngine/binary/binaryFile.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <wrl.h>

namespace SirEngine {
namespace dx12 {
/*
void Dx12RaytracingMesh::load(ID3D12Device *device, tinyobj::attrib_t &attr,
                      tinyobj::shape_t &shape) {
// Allocating and copying data
UINT sz = static_cast<UINT>(attr.vertices_ids.size());
render_index_size = static_cast<uint32_t>(attr.vertices_ids.size());
int STRIDE;
m_cpu_vtx_data = attr.vertices_ids;

STRIDE = 8;
m_stride = STRIDE;
m_vertexs.resize(sz * STRIDE);
m_vertexCount = sz;
m_vertexSize = STRIDE * sizeof(float);
m_indexCount = (render_index_size);
m_indexSize = sizeof(uint16_t);
// allocate memory for index buffer
render_index.resize(render_index_size);
const float *const sourceVtx = attr.vertices.data();
const float *const sourceNorm = attr.normals.data();
const float *const sourceUv = attr.texcoords.data();

float *const vtx = m_vertexs.data();
uint16_t *idx = render_index.data();
//#pragma omp parallel for
for (UINT i = 0; i < sz; ++i) {
const auto vtx_id = attr.vertices_ids[i];
const auto uv_id = attr.text_coords_ids[i];
const auto n_id = attr.normals_ids[i];
const auto curr = i * STRIDE;

vtx[curr + 0] = sourceVtx[vtx_id * 3];
vtx[curr + 1] = sourceVtx[vtx_id * 3 + 1];
vtx[curr + 2] = sourceVtx[vtx_id * 3 + 2];
vtx[curr + 3] = 1.0f;

vtx[curr + 4] = sourceNorm[n_id * 3];
vtx[curr + 5] = sourceNorm[n_id * 3 + 1];
vtx[curr + 6] = sourceNorm[n_id * 3 + 2];
vtx[curr + 7] = 0.0f;

//vtx[curr + 8] = sourceUv[uv_id * 2];
//vtx[curr + 9] = sourceUv[uv_id * 2 + 1];
//vtx[curr + 10] = 0.0f;
//vtx[curr + 11] = 0.0f;
//
//    idx[i] = i;
//  }

m_stride = STRIDE;

system::utils::AllocateUploadBuffer(device, render_index.data(),
                              render_index_size * sizeof(uint16_t),
                              &idxdata);
system::utils::AllocateUploadBuffer(device, m_vertexs.data(),
                              sz * STRIDE * sizeof(float), &vsdata);

m_bufferIDX.resource = idxdata;
m_bufferVS.resource = vsdata;
}
*/

inline void AllocateUploadBuffer(ID3D12Device *pDevice, void *pData,
                                 UINT64 datasize, ID3D12Resource **ppResource,
                                 const wchar_t *resourceName = nullptr) {
  auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
  HRESULT res = pDevice->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(ppResource));

  // if (FAILED(res)) {
  //  std::cout << "ERROR ALLOCATING UPLOAD BUFFER" << std::endl;
  //}
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

void Dx12RaytracingMesh::loadFromFile(ID3D12Device *device,
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
  float *vertexData = (float *)(data.data() + sizeof(BinaryFileHeader));
  float *indexData = (float *)(data.data() + sizeof(BinaryFileHeader) +
                               mapper->vertexDataSizeInByte);

  m_defaultVertex = createDefaultBuffer(DX12Handles::device,
                      DX12Handles::frameCommand->commandList, vertexData,
                      sz * m_stride * sizeof(float), m_uploadVertex);
  m_defaultIndex= createDefaultBuffer(DX12Handles::device,
                      DX12Handles::frameCommand->commandList, indexData,
                      render_index_size * sizeof(int), m_uploadIndex);

  // m_vertexBuffer = getVertexBuffer(sz * m_stride * sizeof(float),
  // vertexData); m_indexBuffer = getIndexBuffer(render_index_size *
  // sizeof(int), indexData);

  // AllocateUploadBuffer(device, render_index.data(),
  //                     render_index_size * sizeof(int), &idxdata);
  // AllocateUploadBuffer(device, m_vertexs.data(), sz * m_stride *
  // sizeof(float),
  //                     &vsdata);

  /*
std::vector<tinyobj::shape_t> shapes;
std::vector<tinyobj::material_t> materials;
tinyobj::attrib_t data;
std::string err;
tinyobj::LoadObj(&data, &shapes, &materials, &err, path.c_str(), 0,
true); load(device, data, shapes[0]);

heap->createBufferSRV(
&(m_bufferIDX),
getIndexCount() /
    2 // getIndexCount return the numeber of 16 bit elements,
      // srv expect the 32 bit number of elements, so dividing by 2
,
0);
heap->createBufferSRV(&(m_bufferVS), getVertexCount(), getVertexSize());
*/
}

/*
void Dx12RaytracingMesh::loadExternal(ID3D12Device *device,
                                      ID3D12Resource *idxData,
                                      ID3D12Resource *vsData, int stride,
                                      int idxCount, int vtxCount,
                                      system::DescriptorHeap *heap) {

  m_stride = stride;
  m_vertexSize = sizeof(float) * stride;
  m_vertexCount = vtxCount;
  m_indexCount = idxCount;
  m_indexSize = sizeof(uint16_t);

  // TODO this is horrible, we don't need to keep track of the pointer twice,
  // since is already stored into buffer resource
  idxdata = idxData;
  vsdata = vsData;

  m_bufferIDX.resource = idxdata;
  m_bufferVS.resource = vsdata;
  // getIndexCount return the numeber of 16 bit elements,
  // srv expect the 32 bit number of elements, so dividing by 2
  int idxCountIn32Bits = getIndexCount() / 2;
  heap->createBufferSRV(&(m_bufferIDX), idxCountIn32Bits, 0);
  heap->createBufferSRV(&(m_bufferVS), getVertexCount(), getVertexSize());
}
*/

/*
void Dx12RaytracingMesh::loadExternalCPU(ID3D12Device* device, uint16_t*
idxData, float* vsData, int stride, int idxCount, int vtxCount,
system::DescriptorHeap * heap)
{
  m_stride = stride;
  m_vertexSize = sizeof(float) * stride;
  m_vertexCount = vtxCount;
  m_indexCount = idxCount;
  m_indexSize = sizeof(uint16_t);

  system::utils::AllocateUploadBuffer(device, idxData,
                                      idxCount* sizeof(uint16_t),
                                      &idxdata);
  system::utils::AllocateUploadBuffer(device, vsData,
                                      vtxCount* m_stride* sizeof(float),
&vsdata);

  m_bufferIDX.resource = idxdata;
  m_bufferVS.resource = vsdata;

  // getIndexCount return the numeber of 16 bit elements,
  // srv expect the 32 bit number of elements, so dividing by 2
  int idxCountIn32Bits = getIndexCount() / 2;
  heap->createBufferSRV(&(m_bufferIDX), idxCountIn32Bits, 0);
  heap->createBufferSRV(&(m_bufferVS), getVertexCount(), getVertexSize());

}
*/

void Dx12RaytracingMesh::translate(float x, float y, float z) {
  DirectX::XMFLOAT4X4 meshTView;
  XMStoreFloat4x4(&meshTView, transform);
  meshTView._41 = x;
  meshTView._42 = y;
  meshTView._43 = z;
  transform = XMLoadFloat4x4(&meshTView);
}
} // namespace dx12
} // namespace SirEngine
