
#include "SirEngine/animation/animationManager.h"

#include <SirEngine/fileUtils.h>
#undef max
#include "SirEngine/animation/animationClip.h"
#include "SirEngine/animation/animationLoopPlayer.h"
#include "SirEngine/animation/skeleton.h"
#include "luaStatePlayer.h"

#include <string>

namespace SirEngine {
static const std::string TYPE_KEY = "type";
static const std::string SKELETON_KEY = "skeleton";
static const std::string ASSET_NAME_KEY = "assetName";
static const std::string ANIMATION_CLIP_KEY = "animationClip";
static const std::string ANIMATION_CONFIG_TYPE_SIMPLE_LOOP =
    "animationLoopPlayer";
static const std::string ANIMATION_CONFIG_TYPE_LUA_STATE = "luaStatePlayer";
static const std::string ANIMATION_CONFIG_NAME_KEY = "name";

AnimationPlayer *loadAnimationLoopPlayer(AnimationManager *manager,
                                         nlohmann::json &jObj) {
  auto *loop = new AnimationLoopPlayer();
  loop->init(manager, jObj);
  return loop;
}
AnimationPlayer *loadLuaStatePlayer(AnimationManager *manager,
                                    nlohmann::json &jObj) {
  auto *loop = new LuaStatePlayer();
  loop->init(manager, jObj);
  return loop;
}

AnimationConfigHandle
AnimationManager::loadAnimationConfig(const char *path, const char *assetName) {

  AnimationConfigHandle earlyHandle{};
  const bool alreadyLoaded = m_nameToConfigHandle.get(assetName, earlyHandle);
  if (alreadyLoaded) {
    return earlyHandle;
  }

  // TODO move this to resource compiler
  auto configJson = getJsonObj(path);
  std::string empty;

  const std::string type = getValueIfInJson(configJson, TYPE_KEY, empty);

  AnimationPlayer *player = nullptr;
  // new method to load animations, this can be extended in the future if needed
  // to allow user defined loaders for new types etc
  if (type == ANIMATION_CONFIG_TYPE_SIMPLE_LOOP) {
    player = loadAnimationLoopPlayer(this, configJson);
  } else if (type == ANIMATION_CONFIG_TYPE_LUA_STATE) {
    player = loadLuaStatePlayer(this, configJson);
  } else {
    assert(0 && "uknown animation player type");
  }

  registerState(player);
  //// allocating anim state;
  // auto *animState = new AnimState();
  // animState->m_name = clip->m_name;
  // animState->m_clip = clip;
  // animState->m_pose = namedPose;
  // animState->m_multiplier = 1.0f;
  // animState->m_loop = true,
  // animState->m_globalStartStamp = m_animClock.getTicks();
  // AnimationConfig config{clip, skeleton, animState};
  const AnimationConfigHandle handle{configIndex++};
  m_handleToConfig.insert(handle.handle, player);
  m_nameToConfigHandle.insert(assetName, handle);
  return handle;
}

void AnimationManager::init() {
  // build up the keyword mapping
  m_keywordRegisterMap.insert(
      "l_foot_down", static_cast<int>(ANIM_CLIP_KEYWORDS::L_FOOT_DOWN));
  m_keywordRegisterMap.insert(
      "r_foot_down", static_cast<int>(ANIM_CLIP_KEYWORDS::R_FOOT_DOWN));
}

AnimationClip *AnimationManager::loadAnimationClip(const char *name,
                                                   const char *path) {

  AnimationClip *clip = getCachedAnimationClip(name);
  if (clip == nullptr) {
    // TODO fix naked
    clip = new AnimationClip();
    const bool res = clip->initialize(path);
    m_animationClipCache.insert(name, clip);
    return res ? clip : nullptr;
  }
  return clip;
}

Skeleton *AnimationManager::loadSkeleton(const char *name, const char *path) {

  Skeleton *skeleton = getCachedSkeleton(name);
  if (skeleton == nullptr) {
    // TODO fix naked
    auto *sk = new Skeleton();
    const bool res = sk->loadFromFile(path);
    m_skeletonCache.insert(name, skeleton);
    return res ? sk : nullptr;
  }
  return skeleton;
}

SkeletonPose *
AnimationManager::getSkeletonPose(const Skeleton *skeleton) const {

  // allocating the pose, it is a simple struct with some pointers in it,
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

void AnimationManager::registerState(AnimationPlayer *state) {
  m_activeAnims.pushBack(state);
}

void AnimationManager::evaluate() const {
  // animation works on a global clock, it is slightly harder
  // but makes easier to sync animations, we grab the time stamp in
  // nanoseconds and pass it along for update, most likely will be converted
  // in seconds. we might evaluate if to convert that upfront
  const long long stamp = m_animClock.getTicks();
  // converting from clock resolution to nanoseconds that s what the
  // anim system uses before converting to seconds
  const std::chrono::nanoseconds nano{stamp};

  // evaluates all the animations
  const uint32_t animCount = m_activeAnims.size();
  for (uint32_t i = 0; i < animCount; ++i) {
    m_activeAnims[i]->evaluate(nano.count());
  }
}
} // namespace SirEngine
