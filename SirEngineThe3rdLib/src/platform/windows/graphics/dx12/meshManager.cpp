#include "platform/windows/graphics/dx12/meshManager.h"

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"

namespace SirEngine {
namespace dx12 {
void MeshManager::clearUploadRequests() {

  auto id = GLOBAL_FENCE->GetCompletedValue();
  // uint32_t freed = 0;
  int requestSize = m_uploadRequests.size() - 1;
  int stackTopIdx = requestSize;
  for (int i = requestSize; i >= 0; --i) {
    MeshUploadResource &upload = m_uploadRequests[i];
    if (upload.fence < id) {
      // we can free the memory
      upload.uploadVertexBuffer->Release();
      upload.uploadIndexBuffer->Release();
      if (stackTopIdx != i) {
        // lets copy
        m_uploadRequests[i] = m_uploadRequests[stackTopIdx];
      }
      --stackTopIdx;
    }
  }
  // resizing the vector
  m_uploadRequests.resize(stackTopIdx + 1);
}

static ID3D12Resource *createDefaultBuffer(ID3D12Device *device,
                                           ID3D12GraphicsCommandList *cmdList,
                                           const void *initData,
                                           const UINT64 byteSize,
                                           ID3D12Resource **uploadBuffer) {
  ID3D12Resource *defaultBuffer = nullptr;

  // Create the actual default buffer resource.
  HRESULT res = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(byteSize), D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&defaultBuffer));
  assert(SUCCEEDED(res));

  // In order to copy CPU memory data into our default buffer, we need to create
  // an intermediate upload heap.
  res = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer));
  assert(SUCCEEDED(res));

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

MeshHandle MeshManager::loadMesh(const char *path, uint32_t runtimeIndex,
                                 MeshRuntime *runtimeMemory) {

  bool res = fileExists(path);
  assert(res);
  // lets check whether or not the mesh has been loaded already
  const std::string name = getFileName(path);
  MeshData *meshData = nullptr;
  MeshHandle handle = {0};
  auto found = m_nameToHandle.find(name);
  if (found == m_nameToHandle.end()) {
    std::vector<char> bindaryData;
    readAllBytes(path, bindaryData);

    auto mapper = getMapperData<ModelMapperData>(bindaryData.data());

    uint32_t stride = mapper->strideInByte / sizeof(float);
    // creating the buffers
    int vertexCount = mapper->vertexDataSizeInByte / mapper->strideInByte;
    uint32_t indexCount = mapper->indexDataSizeInByte / sizeof(int);

    // lets get the vertex data
    auto *vertexData = reinterpret_cast<float *>(bindaryData.data() +
                                                 sizeof(BinaryFileHeader));
    auto *indexData =
        reinterpret_cast<int *>(bindaryData.data() + sizeof(BinaryFileHeader) +
                                mapper->vertexDataSizeInByte);

    // upload the data on the GPU
    uint32_t index;

    meshData = &m_meshPool.getFreeMemoryData(index);
    meshData->indexCount = indexCount;
    meshData->vertexCount = vertexCount;
    meshData->stride = stride;
    MeshUploadResource upload;

    FrameCommand *currentFC = &CURRENT_FRAME_RESOURCE->fc;
    meshData->vertexBuffer = createDefaultBuffer(
        DEVICE, currentFC->commandList, vertexData,
        vertexCount * stride * sizeof(float), &upload.uploadVertexBuffer);
    meshData->indexBuffer = createDefaultBuffer(
        DEVICE, currentFC->commandList, indexData, indexCount * sizeof(int),
        &upload.uploadIndexBuffer);

    // set a signal for the resource.
    upload.fence = dx12::insertFenceToGlobalQueue();
    m_uploadRequests.push_back(upload);

    // data is now loaded need to create handle etc
    handle = MeshHandle{(MAGIC_NUMBER_COUNTER << 16) | index};
    meshData->magicNumber = MAGIC_NUMBER_COUNTER;

    // storing the handle and increasing the magic count
    m_nameToHandle[name] = handle;
    ++MAGIC_NUMBER_COUNTER;
  } else {

    SE_CORE_INFO("Mesh already loaded, returning handle:{0}", name);
    // we already loaded the mesh so we can just get the handle and index data
    uint32_t index = getIndexFromHandle(found->second);
    meshData = &m_meshPool[index];
    handle = found->second;
  }

  // build the runtime mesh
  MeshRuntime &runM = runtimeMemory[runtimeIndex];
  runM.indexCount = meshData->indexCount;
  runM.vview = getVertexBufferView(handle);
  runM.iview = getIndexBufferView(handle);

  return handle;
}

} // namespace dx12
} // namespace SirEngine
