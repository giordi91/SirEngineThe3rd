
#include "SirEngine/animation/animationManager.h"

#include <SirEngine/fileUtils.h>
#undef max
#include "SirEngine/animation/animationClip.h"
#include "SirEngine/animation/skeleton.h"
#include <string>

namespace SirEngine {
static const std::string TYPE_KEY = "type";
static const std::string SKELETON_KEY = "skeleton";
static const std::string ASSET_NAME_KEY = "assetName";
static const std::string ANIMATION_CLIP_KEY = "animationClip";
static const std::string ANIMATION_CONFIG_TYPE = "animationConfig";
static const std::string ANIMATION_CONFIG_NAME_KEY = "name";

AnimationConfigHandle AnimationManager::loadAnimationConfig(const char *path) {
  auto configJson = getJsonObj(path);
  std::string empty("");

  const std::string type = getValueIfInJson(configJson, TYPE_KEY, empty);
  assert(type == ANIMATION_CONFIG_TYPE);

  const std::string configName =
      getValueIfInJson(configJson, ANIMATION_CONFIG_NAME_KEY, empty);

  assert(!configName.empty());
  // checking if is cached by any chance
  // this is a string so we need to hash it
  uint64_t configNameHash =
      hashString(configName.c_str(), static_cast<uint32_t>(configName.size()));

  AnimationConfigHandle earlyHandle{};
  bool found = m_nameToConfigHandle.get(configNameHash, earlyHandle);
  if (found) {
    return earlyHandle;
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
  AnimationClip *clip = getCachedAnimationClip(animationClipFile.c_str(),
                                               animationClipFile.size());
  if (clip == nullptr) {
    clip = loadAnimationClip(animationClipFile.c_str());
    uint64_t hashClip =
        hashString(animationClipFile.c_str(), animationClipFile.size());
    m_animationClipCache.insert(hashClip, clip);
  }
  assert(clip != nullptr);

  // skeleton
  // checking if the skeleton is already cached, if not load it
  const std::string skeletonFileName = getFileName(skeletonFile);
  Skeleton *skeleton =
      getCachedSkeleton(skeletonFileName.c_str(), skeletonFileName.size());
  if (skeleton == nullptr) {
    skeleton = loadSkeleton(skeletonFile.c_str());
    uint64_t hashSkeleton =
        hashString(skeletonFileName.c_str(), skeletonFileName.size());
    m_skeletonCache.insert(hashSkeleton, skeleton);
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
  m_handleToConfig.insert(handle.handle, config);
  m_nameToConfigHandle.insert(configNameHash, handle);
  return handle;
}

AnimationClip *AnimationManager::loadAnimationClip(const char *path) {
  // TODO fix naked
  auto *clip = new AnimationClip();
  const bool res = clip->initialize(path);
  assert(clip != nullptr);
  return res ? clip : nullptr;
}

Skeleton *AnimationManager::loadSkeleton(const char *path) {
  // TODO fix naked
  auto *sk = new Skeleton();
  const bool res = sk->loadFromFile(path);
  return res ? sk : nullptr;
}

SkeletonPose *AnimationManager::getNamedSkeletonPose(const char *name) {

  const uint32_t nameLen = strlen(name);
  auto *sk = getCachedSkeleton(name, nameLen);
  assert(sk != nullptr);

  // TODO this cache is actually bad, what happen if we have multiple characters
  // pointing to same skeleton?
  const uint64_t hash = hashString(name, nameLen);
  SkeletonPose *pose = nullptr;
  const bool found = m_namedPosesMap.get(hash, pose);
  if (found) {
    return pose;
  }

  // allocate one
  // TODO fix naked
  pose = new SkeletonPose();
  pose->m_skeleton = sk;
  pose->m_localPose.resize(sk->m_jointCount);
  pose->m_globalPose.resize(sk->m_jointCount);
  m_namedPosesMap.insert(hash, pose);
  return pose;
}

void AnimationManager::registerState(AnimState *state) {
  m_activeAnims.pushBack(state);
}

void AnimationManager::evaluate() {
  // animation works on a global clock, it is slightly harder
  // but makes easier to sync animations, we grab the time stam in
  // nanoseconds and pass it along for update, most likely will be converted
  // in seconds. we might evaluate if to convert that upfront
  const long long stamp = m_animClock.getTicks();
  // converting from clock resolution to nanoseconds that s what the
  // anim system uses before converting to seconds
  const std::chrono::nanoseconds nano{stamp};

  // evaluates all the animations
  const uint32_t animCount = m_activeAnims.size();
  for (uint32_t i = 0; i < animCount; ++i) {
    m_activeAnims[i]->updateGlobalByAnim(nano.count());
  }
}
} // namespace SirEngine