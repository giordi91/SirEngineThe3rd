#include "platform/windows/graphics/dx12/dx12MeshManager.h"

#include "SirEngine/io/binaryFile.h"
#include "SirEngine/io/fileUtils.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/dx12BufferManager.h"

namespace SirEngine::dx12 {

MeshHandle Dx12MeshManager::loadMesh(const char *path) {
  SE_CORE_INFO("Loading mesh {0}", path);
  const bool res = fileExists(path);
  assert(res);
  // lets check whether or not the mesh has been loaded already
  const std::string name = getFileName(path);
  MeshData *meshData;
  MeshHandle handle{};

  bool found = m_nameToHandle.get(name.c_str(), handle);
  if (!found) {
    std::vector<char> binaryData;
    readAllBytes(path, binaryData);

    const auto *const mapper =
        getMapperData<ModelMapperData>(binaryData.data());

    const uint32_t indexCount = mapper->indexDataSizeInByte / sizeof(int);

    // lets get the vertex data
    auto *vertexData =
        reinterpret_cast<float *>(binaryData.data() + sizeof(BinaryFileHeader));
    auto *indexData =
        reinterpret_cast<int *>(binaryData.data() + sizeof(BinaryFileHeader) +
                                mapper->vertexDataSizeInByte);

    // upload the data on the GPU
    uint32_t index;

    meshData = &m_meshPool.getFreeMemoryData(index);
    meshData->indexCount = indexCount;
    meshData->vertexCount = mapper->vertexCount;

    uint32_t totalSize = indexCount * sizeof(int);
    meshData->idxBuffHandle = dx12::BUFFER_MANAGER->allocate(
        totalSize, indexData, "", totalSize / sizeof(int), sizeof(int),
        BufferManager::BUFFER_FLAGS_BITS::GPU_ONLY);
    meshData->indexBuffer =
        dx12::BUFFER_MANAGER->getNativeBuffer(meshData->idxBuffHandle);

    // load bounding box
    glm::vec3 minP = {mapper->boundingBox[0], mapper->boundingBox[1],
                      mapper->boundingBox[2]};
    glm::vec3 maxP = {mapper->boundingBox[3], mapper->boundingBox[4],
                      mapper->boundingBox[5]};
    BoundingBox box{minP, maxP};
    meshData->entityID = m_boundingBoxes.size();
    m_boundingBoxes.pushBack(box);

    // data is now loaded need to create handle etc
    handle = MeshHandle{(MAGIC_NUMBER_COUNTER << 16) | index};
    meshData->magicNumber = MAGIC_NUMBER_COUNTER;

    // build the runtime mesh
    Dx12MeshRuntime meshRuntime{};
    meshRuntime.indexCount = meshData->indexCount;

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = meshData->indexBuffer->GetGPUVirtualAddress();
    ibv.Format = DXGI_FORMAT_R32_UINT;
    ibv.SizeInBytes = meshData->indexCount * sizeof(int);
    meshRuntime.iview = ibv;
    meshRuntime.positionRange = mapper->positionRange;
    meshRuntime.normalsRange = mapper->normalsRange;
    meshRuntime.uvRange = mapper->uvRange;
    meshRuntime.tangentsRange = mapper->tangentsRange;

    // storing the handle and increasing the magic count
    m_nameToHandle.insert(name.c_str(), handle);
    ++MAGIC_NUMBER_COUNTER;

    BufferHandle positionsHandle = dx12::BUFFER_MANAGER->allocate(
        mapper->vertexDataSizeInByte, vertexData, "",
        mapper->vertexDataSizeInByte / 4, sizeof(float),
        BufferManager::BUFFER_FLAGS_BITS::GPU_ONLY);

    meshRuntime.bufferHandle = positionsHandle;

    meshData->meshRuntime = meshRuntime;
  }
  SE_CORE_INFO("Mesh already loaded, returning handle:{0}", name);
  return handle;
}
}  // namespace SirEngine::dx12
