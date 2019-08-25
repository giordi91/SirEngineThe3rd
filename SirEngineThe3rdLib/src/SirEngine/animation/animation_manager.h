#pragma once
//#include "rendering/skinCluster.h"
#include "SirEngine/clock.h"
#include <unordered_map>
#include <vector>

namespace SirEngine {

typedef unsigned int AnimationConfigHandle;

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

class AnimationManager {
  // friend delcaration needed for singleton

public:
  AnimationManager() = default;
  virtual ~AnimationManager() = default;

  // loader functions
  // those functions either get an already loaded json file
  // or the full path to the json to load
  AnimationConfigHandle loadAnimationConfig(const std::string &path);
  void init(){};

  inline AnimationConfig getConfig(AnimationConfigHandle handle) {
    auto found = m_handleToConfig.find(handle);
    if (found != m_handleToConfig.end()) {
      return found->second;
    }
    return AnimationConfig{};
  }

  /* This function returns the skeleton pose used for a named skeleton,
   * this is the pose which the animation evaluates on so you can use it
   * for see the valua of the matrix after animation, ready for the skin
   * cluster
   */
  SkeletonPose *getNamedSkeletonPose(const std::string &name);

  // registering an animation state for being evluated
  void registerState(AnimState *state);

  // evaluaets the registered resources which are skins and
  // animations, skins should only be registered for debug
  // purpose, otherwise let the rendering component do the
  // skinnign at render time
  void evaluate();
  inline const AnimClock &getAnimClock() const { return m_animClock; }

private:
  AnimationClip *loadAnimationClip(const std::string &path) const;
  Skeleton *loadSkeleton(const std::string &fullPath) const;

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
  std::unordered_map<AnimationConfigHandle, AnimationConfig> m_handleToConfig;
  std::unordered_map<std::string, AnimationConfigHandle> m_nameToConfigName;
  std::unordered_map<std::string, AnimationClip *> m_animationClipCache;
  std::unordered_map<std::string, Skeleton *> m_skeletonCache;
  std::unordered_map<std::string, SkeletonPose *> m_namedPosesMap;
  unsigned int configIndex = 0;

};
} // namespace SirEngine
