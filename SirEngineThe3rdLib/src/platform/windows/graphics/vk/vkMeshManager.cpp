#include "platform/windows/graphics/vk/vkMeshManager.h"

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkBufferManager.h"

namespace SirEngine::vk {
vk::Buffer VkMeshManager::getVertexBuffer(const MeshHandle &handle) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const MeshData &data = m_meshPool.getConstRef(index);
  return vk::BUFFER_MANAGER->getBufferData(data.vtxBuffHandle);
}

vk::Buffer VkMeshManager::getIndexBuffer(const MeshHandle &handle) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const MeshData &data = m_meshPool.getConstRef(index);
  return vk::BUFFER_MANAGER->getBufferData(data.idxBuffHandle);
}

MeshHandle VkMeshManager::loadMesh(const char *path, bool isInternal) {

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
    meshData->idxBuffHandle = vk::BUFFER_MANAGER->allocate(
        totalSize, indexData, "", totalSize / sizeof(int), sizeof(int),
        BufferManager::BUFFER_FLAGS::INDEX_BUFFER);

    meshData->indexBuffer =
        vk::BUFFER_MANAGER->getNativeBuffer(meshData->idxBuffHandle);

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
    VkMeshRuntime meshRuntime{};
    meshRuntime.indexCount = meshData->indexCount;
    meshRuntime.indexBuffer = meshData->indexBuffer;
    meshRuntime.vertexBuffer = meshData->vertexBuffer;
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

    BufferHandle positionsHandle = vk::BUFFER_MANAGER->allocate(
        mapper->vertexDataSizeInByte, vertexData, "",
        mapper->vertexDataSizeInByte / 4, sizeof(float),
        BufferManager::BUFFER_FLAGS::VERTEX_BUFFER);
    meshData->vtxBuffHandle = positionsHandle;

    meshRuntime.vertexBuffer =
        vk::BUFFER_MANAGER->getNativeBuffer(meshData->vtxBuffHandle);
    meshData->vertexBuffer = meshRuntime.vertexBuffer;

    meshData->meshRuntime = meshRuntime;

  } else {
    SE_CORE_INFO("Mesh already loaded, returning handle:{0}", name);
    // we already loaded the mesh so we can just get the handle and index data
    uint32_t index = getIndexFromHandle(found->second);
    meshData = &m_meshPool[index];
    handle = found->second;
  }

  return handle;
}

void VkMeshManager::bindMesh(MeshHandle handle, VkWriteDescriptorSet *set,
                             VkDescriptorSet descriptorSet,
                             VkDescriptorBufferInfo *info) {
  uint32_t magic = getMagicFromHandle(handle);
  uint32_t idx = getIndexFromHandle(handle);
  const MeshData &data = m_meshPool.getConstRef(idx);

  // actual information of the descriptor, in this case it is our mesh buffer
  info[0].buffer = data.vertexBuffer;
  info[0].offset = data.meshRuntime.positionRange.m_offset;
  info[0].range = data.meshRuntime.positionRange.m_size;
  VkDescriptorBufferInfo bufferInfoN = {};
  info[1].buffer = data.vertexBuffer;
  info[1].offset = data.meshRuntime.normalsRange.m_offset;
  info[1].range = data.meshRuntime.normalsRange.m_size;
  VkDescriptorBufferInfo bufferInfoUV = {};
  info[2].buffer = data.vertexBuffer;
  info[2].offset = data.meshRuntime.uvRange.m_offset;
  info[2].range = data.meshRuntime.uvRange.m_size;

  // Binding 0: Object mesh buffer
  set[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  set[1].dstSet = descriptorSet;
  set[1].dstBinding = 1;
  set[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  set[1].pBufferInfo = &info[0];
  set[1].descriptorCount = 1;

  set[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  set[2].dstSet = descriptorSet;
  set[2].dstBinding = 2;
  set[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  set[2].pBufferInfo = &info[1];
  set[2].descriptorCount = 1;

  set[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  set[3].dstSet = descriptorSet;
  set[3].dstBinding = 3;
  set[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  set[3].pBufferInfo = &info[2];
  set[3].descriptorCount = 1;
}

void VkMeshManager::free(const MeshHandle handle)
{
	assertMagicNumber(handle);
	uint32_t index = getIndexFromHandle(handle);
	MeshData& data = m_meshPool[index];
	vk::BUFFER_MANAGER->free(data.vtxBuffHandle);
	vk::BUFFER_MANAGER->free(data.idxBuffHandle);
	// invalidating magic number
	data.magicNumber = 0;
	// adding the index to the free list
	m_meshPool.free(index);
}
} // namespace SirEngine::vk
