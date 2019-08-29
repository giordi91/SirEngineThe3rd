
#include "SirEngine/animation/animation_manager.h"


#include <SirEngine/fileUtils.h>
#undef max
#include "SirEngine/animation/animationClip.h"
#include "SirEngine/animation/skeleton.h"
//#include "rendering/skinCluster.h"
//#include <json\json.hpp>

#include <string>

namespace SirEngine{
static const std::string TYPE_KEY = "type";
static const std::string SKELETON_KEY = "skeleton";
static const std::string ASSET_NAME_KEY = "assetName";
static const std::string ANIMATION_CLIP_KEY = "animationClip";
static const std::string ANIMATION_CONFIG_TYPE = "animationConfig";
static const std::string ANIMATION_CONFIG_NAME_KEY = "name";

AnimationConfigHandle
AnimationManager::loadAnimationConfig(const std::string &path) {
  auto configJson = getJsonObj(path);
  std::string empty("");

  const std::string type = getValueIfInJson(configJson, TYPE_KEY, empty);
  assert(type == ANIMATION_CONFIG_TYPE);

  const std::string configName =
      getValueIfInJson(configJson, ANIMATION_CONFIG_NAME_KEY, empty);

  assert(!configName.empty());
  // checking if is cached by any chance
  auto found = m_nameToConfigName.find(configName);
  if (found != m_nameToConfigName.end()) {
    return found->second;
  }

  const std::string animationClipFile =
      getValueIfInJson(configJson, ANIMATION_CLIP_KEY, empty);
  const std::string assetNameFile =
      getValueIfInJson(configJson, ASSET_NAME_KEY, empty);
  const std::string skeletonFile =
      getValueIfInJson(configJson, SKELETON_KEY, empty);

  assert(!animationClipFile.empty());

  // animation
  // checking if the animation clip is already cached, if not load it
  const std::string animationClipFileName = getFileName(animationClipFile);
  AnimationClip *clip = getCachedAnimationClip(animationClipFile);
  if (clip == nullptr) {
    clip = loadAnimationClip(animationClipFile);
    m_animationClipCache[clip->m_name] = clip;
  }
  assert(clip != nullptr);

  // skeleton
  // checking if the skeleton is already cached, if not load it
  const std::string skeletonFileName = getFileName(skeletonFile);
  Skeleton *skeleton = getCachedSkeleton(skeletonFileName);
  if (skeleton == nullptr) {
    skeleton = loadSkeleton(skeletonFile);
    m_skeletonCache[skeleton->m_name] = skeleton;
  }
  assert(skeleton != nullptr);

  // allocating named pose
  auto *namedPose = getNamedSkeletonPose(skeleton->m_name);

  // allocating anim state;
  auto *animState = new AnimState();
  animState->name = clip->m_name;
  animState->clip = clip;
  animState->m_pose = namedPose, animState->multiplier = 1.0f,
  animState->m_loop = true,
  animState->globalStartStamp = m_animClock.getTicks();

  AnimationConfig config{clip, skeleton, animState};
  AnimationConfigHandle handle{configIndex++};
  m_handleToConfig[handle] = config;
  return handle;
}

AnimationClip *
AnimationManager::loadAnimationClip(const std::string &path) const {
  auto *clip = new AnimationClip();
  bool res = clip->initialize(path.c_str());
  assert(clip != nullptr);
  return res == true ? clip : nullptr;
}

Skeleton *AnimationManager::loadSkeleton(const std::string &path) const {
  auto *sk = new Skeleton();
  bool res = sk->loadFromFile(path.c_str());
  return res == true ? sk : nullptr;
}

SkeletonPose *AnimationManager::getNamedSkeletonPose(const std::string &name) {

  auto *sk = getCachedSkeleton(name);
  assert(sk != nullptr);

  const auto found = m_namedPosesMap.find(name);
  if (found != m_namedPosesMap.end()) {
    return found->second;
  }

  // allocate one
  auto *pose = new SkeletonPose();
  pose->m_skeleton = sk;
  pose->m_localPose.resize(sk->m_jointCount);
  pose->m_globalPose.resize(sk->m_jointCount);
  m_namedPosesMap[name] = pose;
  return pose;
}

void AnimationManager::registerState(AnimState *state) {
  m_activeAnims.push_back(state);
}

void AnimationManager::evaluate() {
  // animation works on a global clock, it is slightly harder
  // but makes easier to sync animations, we grab the time stam in
  // nanoseconds and pass it along for update, most likely will be converted
  // in seconds. we might evaluate if to convert that upfront
  auto stamp = m_animClock.getTicks();
  // converting from clock resolution to nanoesconds that s what the
  // anim system uses before converting to seconds
  std::chrono::nanoseconds nano{stamp};

  // evaluates all the animations
  for (auto &s : m_activeAnims) {
    s->updateGlobalByAnim(nano.count());
  }
}
} // namespace animation
