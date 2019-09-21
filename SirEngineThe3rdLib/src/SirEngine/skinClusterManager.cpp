#include "SirEngine/skinClusterManager.h"
#include "SirEngine/animation/animationManager.h"
#include "SirEngine/animation/skeleton.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/bufferManager.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/stackAllocator.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/bufferManagerDx12.h"
#include "SirEngine/animation/animationClip.h"

namespace SirEngine {

static const int VERTEX_INFLUENCE_COUNT = 6;
static const float SKIN_TOTAL_WEIGHT_TOLERANCE = 0.001f;

SkinHandle
SkinClusterManager::loadSkinCluster(const char *path,
                                    AnimationConfigHandle animHandle) {

  SE_CORE_INFO("Loading skin {0}", path);
  const bool res = fileExists(path);
  assert(res);
  // lets check whether or not the mesh has been loaded already
  const std::string name = getFileName(path);
  const auto found = m_nameToHandle.find(name);
  if (found == m_nameToHandle.end()) {
    std::vector<char> binaryData;
    readAllBytes(path, binaryData);

    const auto mapper = getMapperData<SkinMapperData>(binaryData.data());

    // lets get the vertex data
    auto *joints =
        reinterpret_cast<int *>(binaryData.data() + sizeof(BinaryFileHeader));
    auto *weights =
        reinterpret_cast<float *>(binaryData.data() + sizeof(BinaryFileHeader) +
                                  mapper->jointsSizeInByte);

    // TODO fix buffer names
    // TODO I think those buffers are in the uplaod heap, they are not
    BufferHandle influecesHandle = globals::BUFFER_MANAGER->allocate(
        mapper->jointsSizeInByte, joints, "",
        mapper->jointsSizeInByte / sizeof(int), sizeof(int), false);
    BufferHandle weightsHandle = globals::BUFFER_MANAGER->allocate(
        mapper->weightsSizeInByte, weights, "",
        mapper->weightsSizeInByte / sizeof(float), sizeof(float), false);

    // now we need to generate the buffer for the matrices we are going to use
    const AnimationConfig animConfig =
        globals::ANIMATION_MANAGER->getConfig(animHandle);
    int jointCount = animConfig.m_skeleton->m_jointCount;
    BufferHandle matricesHandle = globals::BUFFER_MANAGER->allocateUpload(
        jointCount * sizeof(float) * 16, "");

    uint32_t index;
    SkinData &data = m_skinPool.getFreeMemoryData(index);
    data.animHandle = animHandle;
    data.influencesBuffer = influecesHandle;
    data.weightsBuffer = weightsHandle;
    data.matricesBuffer = matricesHandle;

    // data is now loaded need to create handle etc
    const SkinHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
    data.magicNumber = MAGIC_NUMBER_COUNTER;
    ++MAGIC_NUMBER_COUNTER;

    m_nameToHandle[name] = handle;

    return handle;
  }
  return found->second;
}

void SkinClusterManager::uploadDirtyMatrices() {
  for (const std::pair<std::string, SkinHandle> element : m_nameToHandle) {
    SkinHandle handle = element.second;
    int index = getIndexFromHandle(handle);
    const SkinData &data = m_skinPool.getConstRef(index);

    void *mappedData = dx12::BUFFER_MANAGER->getMappedData(data.matricesBuffer);
    assert(mappedData != nullptr);

    AnimationConfig animConfig =
        globals::ANIMATION_MANAGER->getConfig(data.animHandle);
    const auto &matricesDataToCopy =
        animConfig.m_anim_state->m_pose->m_globalPose;
    memcpy(mappedData, matricesDataToCopy.data(),
           matricesDataToCopy.size() * sizeof(float) * 16);
  }
}
} // namespace SirEngine
