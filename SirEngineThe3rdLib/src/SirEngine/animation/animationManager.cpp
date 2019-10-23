
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

  // TODO move this to resource compiler
  auto configJson = getJsonObj(path);
  std::string empty;

  const std::string type = getValueIfInJson(configJson, TYPE_KEY, empty);
  assert(type == ANIMATION_CONFIG_TYPE);

  const std::string configName =
      getValueIfInJson(configJson, ANIMATION_CONFIG_NAME_KEY, empty);

  assert(!configName.empty());
  // checking if is cached by any chance
  AnimationConfigHandle earlyHandle{};
  bool found = m_nameToConfigHandle.get(configName.c_str(), earlyHandle);
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
  AnimationClip *clip = getCachedAnimationClip(animationClipFile.c_str());
  if (clip == nullptr) {
    clip = loadAnimationClip(animationClipFile.c_str());
    m_animationClipCache.insert(animationClipFile.c_str(), clip);
  }
  assert(clip != nullptr);

  // skeleton
  // checking if the skeleton is already cached, if not load it
  const std::string skeletonFileName = getFileName(skeletonFile);

  Skeleton *skeleton = getCachedSkeleton(skeletonFileName.c_str());
  if (skeleton == nullptr) {
    skeleton = loadSkeleton(skeletonFile.c_str());
    m_skeletonCache.insert(skeletonFileName.c_str(), skeleton);
  }
  assert(skeleton != nullptr);

  // allocating named pose
  SkeletonPose *namedPose = getSkeletonPose(skeleton);

  // allocating anim state;
  auto *animState = new AnimState();
  animState->m_name = clip->m_name;
  animState->m_clip = clip;
  animState->m_pose = namedPose;
  animState->m_multiplier = 1.0f;
  animState->m_loop = true,
  animState->m_globalStartStamp = m_animClock.getTicks();

  AnimationConfig config{clip, skeleton, animState};
  AnimationConfigHandle handle{configIndex++};
  m_handleToConfig.insert(handle.handle, config);
  m_nameToConfigHandle.insert(configName.c_str(), handle);
  return handle;
}

void AnimationManager::init() {
  // build up the keyword mapping
  m_keywordRegisterMap.insert(
      "l_foot_down", static_cast<int>(ANIM_CLILP_KEYWORDS::L_FOOT_DOWN));
  m_keywordRegisterMap.insert(
      "r_foot_down", static_cast<int>(ANIM_CLILP_KEYWORDS::R_FOOT_DOWN));
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

SkeletonPose *
AnimationManager::getSkeletonPose(const Skeleton *skeleton) const {

  //allocating the pose, it is a simple struct with some pointers in it,
  auto *pose = reinterpret_cast<SkeletonPose *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(SkeletonPose)));

  pose->m_skeleton = skeleton;
  const uint32_t jointCount = pose->m_skeleton->m_jointCount;
  const uint32_t totalSize = sizeof(JointPose) * jointCount +
                        (sizeof(DirectX::XMMATRIX) * jointCount) * 2;
  char *memory = reinterpret_cast<char *>(
      globals::PERSISTENT_ALLOCATOR->allocate(totalSize));
  // local pose is the first pointer so we just need to cast it to correct type
  pose->m_localPose = reinterpret_cast<JointPose *>(memory);
  // the global pose is right after the local pose so we are going to shift for
  // jointCount*JointPoseSize
  pose->m_globalPose = reinterpret_cast<DirectX::XMMATRIX *>(
      memory + (sizeof(JointPose) * jointCount));
  // finally the last array is after m_globalPose, the datatype is already of a
  // matrix so we just go and shift by the joint count
  pose->m_worldMat = pose->m_globalPose + jointCount;
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
