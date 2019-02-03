#include "platform/windows/graphics/dx12/meshManager.h"

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"

namespace SirEngine {
namespace dx12 {

MeshManager::MeshData &MeshManager::getFreeMeshData(uint32_t &index) {
  // if there is not free index
  if (m_freeSlotIndex == 0) {
    m_staticStorage.emplace_back(MeshData{});
    index = static_cast<uint32_t>(m_staticStorage.size() - 1);
    return m_staticStorage[m_staticStorage.size() - 1];
  } else {
    // lets re-use the slot
    MeshData &data = m_staticStorage[m_freeSlots[0]];
    index = m_freeSlots[0];
    // patch the hole
    m_freeSlots[0] = m_freeSlots[m_freeSlotIndex - 1];
    --m_freeSlotIndex;
    return data;
  }
}

static ID3D12Resource *createDefaultBuffer(ID3D12Device *device,
                                    ID3D12GraphicsCommandList *cmdList,
                                    const void *initData, const UINT64 byteSize,
                                    ID3D12Resource **uploadBuffer) {
  ID3D12Resource *defaultBuffer = nullptr;

  // Create the actual default buffer resource.
  HRESULT res = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(byteSize), D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&defaultBuffer));

  // In order to copy CPU memory data into our default buffer, we need to create
  // an intermediate upload heap.
  HRESULT res2 = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer));

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
  cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                  defaultBuffer, D3D12_RESOURCE_STATE_COMMON,
                                  D3D12_RESOURCE_STATE_COPY_DEST));
  UpdateSubresources<1>(cmdList, defaultBuffer, *uploadBuffer, 0, 0, 1,
                        &subResourceData);
  cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                  defaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST,
                                  D3D12_RESOURCE_STATE_GENERIC_READ));

  // Note: uploadBuffer has to be kept alive after the above function calls
  // because the command list has not been executed yet that performs the actual
  // copy. The caller can Release the uploadBuffer after it knows the copy has
  // been executed.

  return defaultBuffer;
}

MeshHandle MeshManager::loadMesh(const char *path) {

  bool res = fileExists(path);

  const std::string name = getFileName(path);
  assert(m_nameToHandle.find(name) == m_nameToHandle.end());

  std::vector<char> bindaryData;
  readAllBytes(path, bindaryData);
  const BinaryFileHeader *h = getHeader(bindaryData.data());

  auto mapper = getMapperData<ModelMapperData>(bindaryData.data());

  uint32_t stride = mapper->strideInByte / sizeof(float);
  // creating the buffers
  int vertexCount = mapper->vertexDataSizeInByte / mapper->strideInByte;
  uint32_t indexCount = mapper->indexDataSizeInByte / sizeof(int);

  // lets get the vertex data
  auto *vertexData =
      reinterpret_cast<float *>(bindaryData.data() + sizeof(BinaryFileHeader));
  auto *indexData =
      reinterpret_cast<int *>(bindaryData.data() + sizeof(BinaryFileHeader) +
                              mapper->vertexDataSizeInByte);

  // upload the data on the GPU
  uint32_t index;
  MeshData &meshData = getFreeMeshData(index);
  meshData.indexCount = indexCount;
  meshData.vertexCount = vertexCount;
  meshData.stride = stride;
  MeshUploadResource upload;

  FrameCommand *currentFC = &CURRENT_FRAME_RESOURCE->fc;
  meshData.vertexBuffer = createDefaultBuffer(
      DEVICE, currentFC->commandList, vertexData, vertexCount * stride * sizeof(float),
      &upload.uploadVertexBuffer);
  meshData.indexBuffer =
      createDefaultBuffer(DEVICE, currentFC->commandList, indexData,
                          indexCount * sizeof(int), &upload.uploadIndexBuffer);

  // set a signal for the resource.
  upload.fence = dx12::insertFenceToGlobalQueue();

  // data is now loaded need to create handle etc
  MeshHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
  meshData.magicNumber = MAGIC_NUMBER_COUNTER;

  ++MAGIC_NUMBER_COUNTER;

  m_nameToHandle[name] = handle;
  return handle;
}

} // namespace dx12
} // namespace SirEngine