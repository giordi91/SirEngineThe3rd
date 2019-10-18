#pragma once

#include "SirEngine/clock.h"
#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/memory/hashMap.h"
#include "SirEngine/memory/resizableVector.h"
#include "SirEngine/memory/stringHashMap.h"

namespace SirEngine {

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

// Anim metadata mostly used for blending, can be extended by user in case is
// needed

//upeasing dll interface for interfaces
template class SIR_ENGINE_API Clock<std::chrono::nanoseconds>;
using AnimClock = Clock<std::chrono::nanoseconds>;
template class SIR_ENGINE_API ResizableVector<AnimState *>;
template class SIR_ENGINE_API HashMap<uint32_t, AnimationConfig, hashUint32>;
template class SIR_ENGINE_API
    HashMap<uint64_t, AnimationConfigHandle, hashUint64>;
template class SIR_ENGINE_API HashMap<uint64_t, AnimationClip *, hashUint64>;
template class SIR_ENGINE_API HashMap<uint64_t, Skeleton *, hashUint64>;
template class SIR_ENGINE_API HashMap<uint64_t, SkeletonPose *, hashUint64>;
template class SIR_ENGINE_API HashMap<const char *, int, hashString32>;
class SIR_ENGINE_API AnimationManager final {

public:
  AnimationManager()
      : m_activeAnims(200), m_handleToConfig(500), m_nameToConfigHandle(500),
        m_animationClipCache(500), m_skeletonCache(50), m_namedPosesMap(500),
        m_keywordRegisterMap(50){};
  ~AnimationManager() = default;

  // loader functions
  // those functions either get an already loaded json file
  // or the full path to the json to load
  AnimationConfigHandle loadAnimationConfig(const char *path);
  [[nodiscard]] static AnimationClip *loadAnimationClip(const char *path);
  void init();

  const ResizableVector<AnimState *> &getAnimStates() const {
    return m_activeAnims;
  }

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
  SkeletonPose *getNamedSkeletonPose(const char *name);

  // registering an animation state for being evaluated
  void registerState(AnimState *state);

  // evaluates the registered resources which are skins and
  // animations, skins should only be registered for debug
  // purpose, otherwise let the rendering component do the
  // skinning at render time
  void evaluate();
  inline const AnimClock &getAnimClock() const { return m_animClock; }
  inline int animationKeywordNameToValue(const char *key) const {
    int value = -1;
    const bool found = m_keywordRegisterMap.get(key, value);
    return found ? value : -1;
  }

private:
  [[nodiscard]] static Skeleton *loadSkeleton(const char *path);

  inline AnimationClip *getCachedAnimationClip(const char *name,
                                               const uint32_t len) const {
    const uint64_t hash = hashString(name, len);
    AnimationClip *clip = nullptr;
    const auto found = m_animationClipCache.get(hash, clip);
    return found ? clip : nullptr;
  };

  inline Skeleton *getCachedSkeleton(const char *name, uint32_t len) const {

    const uint64_t hash = hashString(name, len);
    Skeleton *skeleton = nullptr;
    const auto found = m_skeletonCache.get(hash, skeleton);
    return found ? skeleton : nullptr;
  };

private:
  typedef uint64_t string64;
  // resources to be evaluated per frame
  ResizableVector<AnimState *> m_activeAnims;

  AnimClock m_animClock;
  HashMap<uint32_t, AnimationConfig, hashUint32> m_handleToConfig;
  // TODO why string64 and not directly the string variant? what happened
  // if we get a collision?
  HashMap<string64, AnimationConfigHandle, hashUint64> m_nameToConfigHandle;
  HashMap<string64, AnimationClip *, hashUint64> m_animationClipCache;
  HashMap<string64, Skeleton *, hashUint64> m_skeletonCache;
  HashMap<string64, SkeletonPose *, hashUint64> m_namedPosesMap;

  HashMap<const char *, int, hashString32> m_keywordRegisterMap;
  unsigned int configIndex = 0;
};
} // namespace SirEngine
