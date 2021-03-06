#include "platform/windows/graphics/vk/vkMeshManager.h"

#include "SirEngine/io/binaryFile.h"
#include "SirEngine/io/fileUtils.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkBufferManager.h"

namespace SirEngine::vk {
void VkMeshManager::cleanup() {
  uint32_t count = m_nameToHandle.binCount();
  for (uint32_t i = 0; i < count; ++i) {
    if (m_nameToHandle.isBinUsed(i)) {
      const MeshHandle handle = m_nameToHandle.getValueAtBin(i);
      free(handle);
    }
  }
}

MeshHandle VkMeshManager::loadMesh(const char *path) {
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
    meshData->idxBuffHandle = vk::BUFFER_MANAGER->allocate(
        totalSize, indexData, frameConcatenation(name.c_str(), "-indexBuffer"),
        totalSize / sizeof(int), sizeof(int),
        BufferManager::BUFFER_FLAGS_BITS::INDEX_BUFFER |
            BufferManager::BUFFER_FLAGS_BITS::IS_STATIC |
            BufferManager::BUFFER_FLAGS_BITS::GPU_ONLY);

    meshData->indexBuffer =
        vk::BUFFER_MANAGER->getNativeBuffer(meshData->idxBuffHandle);

    // load bounding box
    glm::vec3 minP = {mapper->boundingBox[0], mapper->boundingBox[1],
                      mapper->boundingBox[2]};
    glm::vec3 maxP = {mapper->boundingBox[3], mapper->boundingBox[4],
                      mapper->boundingBox[5]};
    BoundingBox box{minP, maxP, {}};
    meshData->entityID = static_cast<uint32_t>(m_boundingBoxes.size());
    m_boundingBoxes.pushBack(box);

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
    m_nameToHandle.insert(name.c_str(), handle);
    ++MAGIC_NUMBER_COUNTER;

    BufferHandle positionsHandle = vk::BUFFER_MANAGER->allocate(
        mapper->vertexDataSizeInByte, vertexData,
        frameConcatenation(name.c_str(), "-meshBuffer"),
        static_cast<int>(mapper->vertexDataSizeInByte / 4u), sizeof(float),
        BufferManager::BUFFER_FLAGS_BITS::VERTEX_BUFFER |
            BufferManager::BUFFER_FLAGS_BITS::IS_STATIC |
            BufferManager::BUFFER_FLAGS_BITS::GPU_ONLY);
    meshData->vtxBuffHandle = positionsHandle;

    meshRuntime.vertexBuffer =
        vk::BUFFER_MANAGER->getNativeBuffer(meshData->vtxBuffHandle);
    meshData->vertexBuffer = meshRuntime.vertexBuffer;

    meshData->meshRuntime = meshRuntime;
  }
  SE_CORE_INFO("Mesh already loaded, returning handle:{0}", name);
  return handle;
}

void VkMeshManager::bindMesh(const MeshHandle handle, VkWriteDescriptorSet *set,
                             const VkDescriptorSet descriptorSet,
                             VkDescriptorBufferInfo *info,
                             const uint32_t bindFlags,
                             const uint32_t startIdx) const {
  assertMagicNumber(handle);
  uint32_t idx = getIndexFromHandle(handle);
  const MeshData &data = m_meshPool.getConstRef(idx);

  if ((bindFlags & MESH_ATTRIBUTE_FLAGS::POSITIONS) > 0) {
    // actual information of the descriptor, in this case it is our mesh buffer
    info[0].buffer = data.vertexBuffer;
    info[0].offset = data.meshRuntime.positionRange.m_offset;
    info[0].range = data.meshRuntime.positionRange.m_size;

    // Binding 0: Object mesh buffer
    set[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set[0].dstSet = descriptorSet;
    set[0].dstBinding = startIdx + 0;
    set[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    set[0].pBufferInfo = &info[0];
    set[0].descriptorCount = 1;
  }

  if ((bindFlags & MESH_ATTRIBUTE_FLAGS::NORMALS) > 0) {
    info[1].buffer = data.vertexBuffer;
    info[1].offset = data.meshRuntime.normalsRange.m_offset;
    info[1].range = data.meshRuntime.normalsRange.m_size;

    set[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set[1].dstSet = descriptorSet;
    set[1].dstBinding = startIdx + 1;
    set[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    set[1].pBufferInfo = &info[1];
    set[1].descriptorCount = 1;
  }

  if ((bindFlags & MESH_ATTRIBUTE_FLAGS::UV) > 0) {
    info[2].buffer = data.vertexBuffer;
    info[2].offset = data.meshRuntime.uvRange.m_offset;
    info[2].range = data.meshRuntime.uvRange.m_size;

    set[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set[2].dstSet = descriptorSet;
    set[2].dstBinding = startIdx + 2;
    set[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    set[2].pBufferInfo = &info[2];
    set[2].descriptorCount = 1;
  }
  if ((bindFlags & MESH_ATTRIBUTE_FLAGS::TANGENTS) > 0) {
    info[3].buffer = data.vertexBuffer;
    info[3].offset = data.meshRuntime.tangentsRange.m_offset;
    info[3].range = data.meshRuntime.tangentsRange.m_size;

    set[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set[3].dstSet = descriptorSet;
    set[3].dstBinding = startIdx + 3;
    set[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    set[3].pBufferInfo = &info[3];
    set[3].descriptorCount = 1;
  }
}

void VkMeshManager::free(const MeshHandle handle) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  MeshData &data = m_meshPool[index];
  vk::BUFFER_MANAGER->free(data.vtxBuffHandle);
  vk::BUFFER_MANAGER->free(data.idxBuffHandle);
  // invalidating magic number
  data = {};
  // adding the index to the free list
  m_meshPool.free(index);
}
}  // namespace SirEngine::vk
