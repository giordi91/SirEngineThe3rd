#pragma once
//#include "rendering/skinCluster.h"
#include "SirEngine/clock.h"
#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/memory/hashMap.h"
#include <unordered_map>
#include <vector>

namespace SirEngine {

using AnimClock = Clock<std::chrono::nanoseconds>;
// in anim space forward declare
struct Skeleton;
struct SkeletonPose;
struct AnimationClip;
struct AnimState;

struct AnimationConfig {
  AnimationClip *m_animationClip = nullptr;
  Skeleton *m_skeleton = nullptr;
  AnimState *m_anim_state = nullptr;
};

class AnimationManager final {
  // friend delcaration needed for singleton

public:
  AnimationManager() : m_handleToConfig(500), m_nameToConfigHandle(500){};
  ~AnimationManager() = default;

  // loader functions
  // those functions either get an already loaded json file
  // or the full path to the json to load
  AnimationConfigHandle loadAnimationConfig(const char *path);
  void init(){};

  inline AnimationConfig getConfig(const AnimationConfigHandle handle) const {
    AnimationConfig config{};
    const bool result = m_handleToConfig.get(handle.handle, config);
    assert(result);
    return config;
  }

  // This function returns the skeleton pose used for a named skeleton,
  // this is the pose which the animation evaluates,  so you can use it
  // to see the values of the matrix after animation update, ready
  // for the skin cluster
  SkeletonPose *getNamedSkeletonPose(const std::string &name);

  // registering an animation state for being evaluated
  void registerState(AnimState *state);

  // evaluates the registered resources which are skins and
  // animations, skins should only be registered for debug
  // purpose, otherwise let the rendering component do the
  // skinning at render time
  void evaluate();
  inline const AnimClock &getAnimClock() const { return m_animClock; }

private:
  [[nodiscard]] static AnimationClip *
  loadAnimationClip(const std::string &path);
  [[nodiscard]] static Skeleton *loadSkeleton(const std::string &fullPath);

  inline AnimationClip *getCachedAnimationClip(const std::string &name) const {
    const auto found = m_animationClipCache.find(name);
    if (found != m_animationClipCache.end()) {
      return found->second;
    }
    return nullptr;
  };

  inline Skeleton *getCachedSkeleton(const std::string &name) const {
    const auto found = m_skeletonCache.find(name);
    if (found != m_skeletonCache.end()) {
      return found->second;
    }
    return nullptr;
  };

private:
  // resources to be evaluated per frame
  std::vector<AnimState *> m_activeAnims;

  AnimClock m_animClock;
  // HashMap<uint32_t, AnimationConfig> m_handleToConfig;
  HashMap<uint32_t, AnimationConfig, hashUint32> m_handleToConfig;
  // std::unordered_map<std::string, AnimationConfigHandle>
  // m_nameToConfigHandle;
  HashMap<uint64_t, AnimationConfigHandle, hashUint64> m_nameToConfigHandle;
  std::unordered_map<std::string, AnimationClip *> m_animationClipCache;
  std::unordered_map<std::string, Skeleton *> m_skeletonCache;
  std::unordered_map<std::string, SkeletonPose *> m_namedPosesMap;
  unsigned int configIndex = 0;
};
} // namespace SirEngine
