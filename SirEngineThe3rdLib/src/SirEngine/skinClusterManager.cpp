#include "SirEngine/skinClusterManager.h"
#include "SirEngine/memory/stackAllocator.h"
#include "fileUtils.h"
#include "globals.h"

namespace SirEngine {

static const int VERTEX_INFLUENCE_COUNT = 4;
static const float SKIN_TOTAL_WEIGHT_TOLERANCE = 0.001f;

SkinHandle
SkinClusterManager::loadSkinCluster(const char *path,
                                    AnimationConfigHandle animHandle) {

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

  // now we do have the buffers we need to upload them on the GPU

  return SkinHandle{0};
} // namespace SirEngine
} // namespace SirEngine
