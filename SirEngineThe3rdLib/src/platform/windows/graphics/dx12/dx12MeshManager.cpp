#include "platform/windows/graphics/dx12/dx12MeshManager.h"

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "bufferManagerDx12.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"

namespace SirEngine::dx12 {

MeshHandle Dx12MeshManager::loadMesh(const char *path, bool isInternal) {

  SE_CORE_INFO("Loading mesh {0}", path);
  const bool res = fileExists(path);
  assert(res);
  // lets check whether or not the mesh has been loaded already
  const std::string name = getFileName(path);
  MeshData *meshData;
  MeshHandle handle{};

  const auto found = m_nameToHandle.find(name);
  if (found == m_nameToHandle.end()) {
    std::vector<char> binaryData;
    readAllBytes(path, binaryData);

    const auto mapper = getMapperData<ModelMapperData>(binaryData.data());

    // const uint32_t stride = mapper->strideInByte / sizeof(float);
    //// creating the buffers
    // const uint32_t vertexCount =
    //    mapper->vertexDataSizeInByte / mapper->strideInByte;
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
    // meshData->stride = mapp;

    uint32_t totalSize = mapper->vertexDataSizeInByte;
    meshData->vtxBuffHandle = dx12::BUFFER_MANAGER->allocate(
        totalSize, vertexData, "", totalSize, sizeof(float), false);
    meshData->vertexBuffer =
        dx12::BUFFER_MANAGER->getNativeBuffer(meshData->vtxBuffHandle);

    totalSize = indexCount * sizeof(int);
    meshData->idxBuffHandle = dx12::BUFFER_MANAGER->allocate(
        totalSize, indexData, "", totalSize / sizeof(int), sizeof(int), false);
    meshData->indexBuffer =
        dx12::BUFFER_MANAGER->getNativeBuffer(meshData->idxBuffHandle);

    // TODO if is an internal mesh we don't want to go in the bounding box
    // this needs to be replaced with a proper scene and asset management
    if (!isInternal) {
      // load bounding box
      glm::vec3 minP = {mapper->boundingBox[0], mapper->boundingBox[1],
                        mapper->boundingBox[2]};
      glm::vec3 maxP = {mapper->boundingBox[3], mapper->boundingBox[4],
                        mapper->boundingBox[5]};
      BoundingBox box{minP, maxP};
      meshData->entityID = m_boundingBoxes.size();
      m_boundingBoxes.push_back(box);
    }

    // data is now loaded need to create handle etc
    handle = MeshHandle{(MAGIC_NUMBER_COUNTER << 16) | index};
    meshData->magicNumber = MAGIC_NUMBER_COUNTER;

    // build the runtime mesh
    MeshRuntime meshRuntime{};
    meshRuntime.indexCount = meshData->indexCount;
    meshRuntime.vview = getVertexBufferView(handle);
    meshRuntime.iview = getIndexBufferView(handle);
    meshRuntime.positionRange = mapper->positionRange;
    meshRuntime.normalsRange = mapper->normalsRange;
    meshRuntime.uvRange = mapper->uvRange;
    meshRuntime.tangentsRange = mapper->tangentsRange;

    // storing the handle and increasing the magic count
    m_nameToHandle[name] = handle;
    ++MAGIC_NUMBER_COUNTER;

    // new stuff
    int newVertexCount = mapper->vertexCount;

    int positionSize = newVertexCount * 4 * sizeof(float);
    auto *ptr = (char *)vertexData;

    BufferHandle positionsHandle = dx12::BUFFER_MANAGER->allocate(
        mapper->vertexDataSizeInByte, vertexData, "",
        mapper->vertexDataSizeInByte / 4, sizeof(float), false);
    /*
    BufferHandle positionsHandle = dx12::BUFFER_MANAGER->allocate(
        mapper->positionRange.m_size, ptr + mapper->positionRange.m_offset, "",
        mapper->vertexCount * 4, sizeof(float),
        false);
    BufferHandle normalsHandle = dx12::BUFFER_MANAGER->allocate(
        mapper->normalsRange.m_size, ptr + mapper->normalsRange.m_offset, "",
        mapper->vertexCount * 4, sizeof(float),
        false);
    BufferHandle uvHandle = dx12::BUFFER_MANAGER->allocate(
        mapper->uvRange.m_size, ptr + mapper->uvRange.m_offset, "",
        mapper->vertexCount * 3, sizeof(float), false);
    BufferHandle tangents = dx12::BUFFER_MANAGER->allocate(
        mapper->tangentsRange.m_size, ptr + mapper->tangentsRange.m_offset, "",
        mapper->vertexCount * 4, sizeof(float), false);
        */

    meshRuntime.bufferHandle = positionsHandle;
    // meshRuntime.normals = normalsHandle;
    // meshRuntime.uv = uvHandle;
    // meshRuntime.tangents = tangents;

    meshData->meshRuntime = meshRuntime;
    // BufferHandle influecesHandle = globals::BUFFER_MANAGER->allocate(
    //    mapper->jointsSizeInByte, joints, "",
    //    mapper->jointsSizeInByte / sizeof(int), sizeof(int), false);

  } else {
    SE_CORE_INFO("Mesh already loaded, returning handle:{0}", name);
    // we already loaded the mesh so we can just get the handle and index data
    uint32_t index = getIndexFromHandle(found->second);
    meshData = &m_meshPool[index];
    handle = found->second;
  }

  return handle;
}
} // namespace SirEngine::dx12
