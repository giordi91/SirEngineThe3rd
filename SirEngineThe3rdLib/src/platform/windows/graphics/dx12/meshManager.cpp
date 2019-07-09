#include "platform/windows/graphics/dx12/meshManager.h"

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"

namespace SirEngine::dx12 {
void MeshManager::clearUploadRequests() {

  auto id = GLOBAL_FENCE->GetCompletedValue();
  // uint32_t freed = 0;
  int requestSize = static_cast<int>(m_uploadRequests.size()) - 1;
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
  auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto defaultBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
  HRESULT res = device->CreateCommittedResource(
      &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &defaultBufferDesc,
      D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&defaultBuffer));
  assert(SUCCEEDED(res));

  // In order to copy CPU memory data into our default buffer, we need to create
  // an intermediate upload heap.
  auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
  res = device->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
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
  auto preTransition = CD3DX12_RESOURCE_BARRIER::Transition(
      defaultBuffer, D3D12_RESOURCE_STATE_COMMON,
      D3D12_RESOURCE_STATE_COPY_DEST);
  cmdList->ResourceBarrier(1, &preTransition);
  UpdateSubresources<1>(cmdList, defaultBuffer, *uploadBuffer, 0, 0, 1,
                        &subResourceData);
  auto postTransition = CD3DX12_RESOURCE_BARRIER::Transition(
      defaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST,
      D3D12_RESOURCE_STATE_GENERIC_READ);
  cmdList->ResourceBarrier(1, &postTransition);

  // Note: uploadBuffer has to be kept alive after the above function calls
  // because the command list has not been executed yet that performs the actual
  // copy. The caller can Release the uploadBuffer after it knows the copy has
  // been executed.
  return defaultBuffer;
}

MeshHandle MeshManager::loadMesh(const char *path, MeshRuntime *meshRuntime) {

  SE_CORE_INFO("Loading mesh {0}", path);
  const bool res = fileExists(path);
  assert(res);
  // lets check whether or not the mesh has been loaded already
  const std::string name = getFileName(path);
  MeshData *meshData;
  MeshHandle handle;
  const auto found = m_nameToHandle.find(name);
  if (found == m_nameToHandle.end()) {
    std::vector<char> bindaryData;
    readAllBytes(path, bindaryData);

    const auto mapper = getMapperData<ModelMapperData>(bindaryData.data());

    const uint32_t stride = mapper->strideInByte / sizeof(float);
    // creating the buffers
    const uint32_t vertexCount = mapper->vertexDataSizeInByte / mapper->strideInByte;
    const uint32_t indexCount = mapper->indexDataSizeInByte / sizeof(int);

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

    FrameCommand *currentFc = &CURRENT_FRAME_RESOURCE->fc;
    meshData->vertexBuffer = createDefaultBuffer(
        DEVICE, currentFc->commandList, vertexData,
        vertexCount * stride * sizeof(float), &upload.uploadVertexBuffer);
    meshData->indexBuffer = createDefaultBuffer(
        DEVICE, currentFc->commandList, indexData, indexCount * sizeof(int),
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
  // MeshRuntime &runM = runtimeMemory[runtimeIndex];
  meshRuntime->indexCount = meshData->indexCount;
  meshRuntime->vview = getVertexBufferView(handle);
  meshRuntime->iview = getIndexBufferView(handle);

  return handle;
}
} // namespace SirEngine::dx12
