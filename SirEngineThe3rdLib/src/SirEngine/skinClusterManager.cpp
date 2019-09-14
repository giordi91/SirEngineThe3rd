#include "SirEngine/skinClusterManager.h"
#include "SirEngine/memory/stackAllocator.h"
#include "binary/binaryFile.h"
#include "bufferManager.h"
#include "fileUtils.h"
#include "globals.h"
#include "log.h"
#include "animation/animationManager.h"
#include "animation/skeleton.h"

namespace SirEngine {

static const int VERTEX_INFLUENCE_COUNT = 6;
static const float SKIN_TOTAL_WEIGHT_TOLERANCE = 0.001f;

SkinHandle
SkinClusterManager::loadSkinCluster(const char *path,
                                    AnimationConfigHandle animHandle) {

  /*
// here we load the skincluster
auto sObj = getJsonObj(path);
// getting the skeleton name
auto sk = sObj["skeleton"].get<std::string>();

// allocating space for the joints and the weights
auto data = sObj["data"];
const auto size = sObj["data"].size();
//  auto jnts = static_cast<int *>(
//      m_skin_alloc.allocate(size * VERTEX_INFLUENCE_COUNT * sizeof(int)));
const int countSize = size * VERTEX_INFLUENCE_COUNT;
auto *jnts = reinterpret_cast<int *>(
globals::FRAME_ALLOCATOR->allocate(countSize * sizeof(int)));
auto *weights = reinterpret_cast<float *>(
globals::FRAME_ALLOCATOR->allocate(countSize * sizeof(float)));

// auto weights = static_cast<float *>(
//    m_skin_alloc.allocate(size * VERTEX_INFLUENCE_COUNT * sizeof(float)));

// those two variables are used to keep track
// to where to write in the buffer
int counter = 0;
for (auto d : sObj["data"]) {
assert(d.find("j") != d.end());
assert(d.find("w") != d.end());
nlohmann::basic_json<> j = d["j"];
nlohmann::basic_json<> w = d["w"];
const int id = counter * VERTEX_INFLUENCE_COUNT;

// making sure we have the expected amount of data for each joint
assert(j.size() == VERTEX_INFLUENCE_COUNT);
assert(w.size() == VERTEX_INFLUENCE_COUNT);

#if _DEBUG
float w_total = 0.0f;
#endif
for (int i = 0; i < VERTEX_INFLUENCE_COUNT; ++i) {
assert((id + i) < countSize);
jnts[id + i] = j[i].get<int>();
weights[id + i] = w[i].get<float>();

// checking that the weights don't exceed one
#if _DEBUG
w_total += weights[id + i];
#endif
}

assert((w_total <= (1.0f + SKIN_TOTAL_WEIGHT_TOLERANCE)));

++counter;
}
*/
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
        mapper->weightsSizeInByte/ sizeof(float), sizeof(float), false);

	//now we need to generate the buffer for the matrices we are going to use
	const AnimationConfig animConfig = globals::ANIMATION_MANAGER->getConfig(animHandle);
	int jointCount = animConfig.m_skeleton->m_jointCount;
    BufferHandle matricesHandle = globals::BUFFER_MANAGER->allocateUpload(
        jointCount * sizeof(float)*16,"");

  	

    uint32_t index;
    SkinData &data = m_skinPool.getFreeMemoryData(index);
    data.animHandle = animHandle;
    data.influencesBuffer = influecesHandle;
    data.weightsBuffer = weightsHandle;
	data.matricesBuffer= matricesHandle; 

    // data is now loaded need to create handle etc
    const SkinHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
    data.magicNumber = MAGIC_NUMBER_COUNTER;
    ++MAGIC_NUMBER_COUNTER;

    m_nameToHandle[name] = handle;

	return handle;
  }
  return found->second;
}
} // namespace SirEngine
