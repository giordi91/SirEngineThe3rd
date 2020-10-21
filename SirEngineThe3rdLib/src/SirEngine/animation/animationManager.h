#pragma once


#include "SirEngine/clock.h"
#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/memory/cpu/hashMap.h"
#include "SirEngine/memory/cpu/resizableVector.h"
#include "SirEngine/memory/cpu/stringHashMap.h"

namespace SirEngine {

// in anim space forward declare
struct Skeleton;
struct SkeletonPose;
struct AnimationClip;
class AnimationPlayer;

// struct AnimationConfig {
//  AnimationClip *m_animationClip = nullptr;
//  Skeleton *m_skeleton = nullptr;
//  AnimState *m_anim_state = nullptr;
//};

using AnimClock = Clock<std::chrono::nanoseconds>;

class AnimationManager final {
 public:
  AnimationManager()
      : m_activeAnims(200),
        m_handleToConfig(500),
        m_nameToConfigHandle(50),
        m_animationClipCache(500),
        m_skeletonCache(50),
        m_keywordRegisterMap(50){};
  ~AnimationManager() = default;

  // loader functions
  // those functions either get an already loaded json file
  // or the full path to the json to load
  AnimationConfigHandle loadAnimationConfig(const char *path,
                                            const char *assetName);
  AnimationClip *loadAnimationClip(const char *name, const char *path);
  void init();

  const ResizableVector<AnimationPlayer *> &getAnimStates() const {
    return m_activeAnims;
  }

  inline AnimationPlayer *getConfig(const AnimationConfigHandle handle) const {
    AnimationPlayer *player = nullptr;
    const bool result = m_handleToConfig.get(handle.handle, player);
    assert(result);
    return player;
  }

  // Returns a block of memory that will be used to store the evaluated
  // pose for the given animation, enough memory is allocated to fit the
  // given skeleton
  SkeletonPose *getSkeletonPose(const Skeleton *skeleton) const;

  // registering an animation state for being evaluated
  void registerState(AnimationPlayer *state);

  // evaluates the registered resources which are skins and
  // animations, skins should only be registered for debug
  // purpose, otherwise let the rendering component do the
  // skinning at render time
  void evaluate() const;
  inline const AnimClock &getAnimClock() const { return m_animClock; }
  inline int animationKeywordNameToValue(const char *key) const {
    int value = -1;
    const bool found = m_keywordRegisterMap.get(key, value);
    return found ? value : -1;
  }
  [[nodiscard]] Skeleton *loadSkeleton(const char *name, const char *path);

  inline const AnimationClip *getAnimationClipByName(const char *name) const {
    AnimationClip *clip;
    const bool found = m_animationClipCache.get(name, clip);
    assert(found);
    return clip;
  }

  inline AnimationConfigHandle getConfigHandleFromName(
      const char *configName) const {
    AnimationConfigHandle handle;
    const bool found = m_nameToConfigHandle.get(configName, handle);
    assert(found);
    return handle;
  }

  inline AnimationPlayer *getAnimationPlayer(
      const AnimationConfigHandle handle) const {
    AnimationPlayer *player;
    const bool found = m_handleToConfig.get(handle.handle, player);
    assert(found);
    return player;
  }

 public:
 private:
  inline AnimationClip *getCachedAnimationClip(const char *name) const {
    AnimationClip *clip = nullptr;
    const auto found = m_animationClipCache.get(name, clip);
    return found ? clip : nullptr;
  };
  inline Skeleton *getCachedSkeleton(const char *name) const {
    Skeleton *skeleton = nullptr;
    const auto found = m_skeletonCache.get(name, skeleton);
    return found ? skeleton : nullptr;
  };

 private:
  typedef uint64_t string64;
  // resources to be evaluated per frame
  ResizableVector<AnimationPlayer *> m_activeAnims;

  AnimClock m_animClock;
  HashMap<uint32_t, AnimationPlayer *, hashUint32> m_handleToConfig;
  HashMap<const char *, AnimationConfigHandle, hashString32>
      m_nameToConfigHandle;
  HashMap<const char *, AnimationClip *, hashString32> m_animationClipCache;
  HashMap<const char *, Skeleton *, hashString32> m_skeletonCache;

  HashMap<const char *, int, hashString32> m_keywordRegisterMap;
  unsigned int configIndex = 0;
};
}  // namespace SirEngine
