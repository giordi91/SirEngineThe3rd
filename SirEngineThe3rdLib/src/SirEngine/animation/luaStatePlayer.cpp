#include "SirEngine/animation/luaStatePlayer.h"
#include "SirEngine/animation/animationManager.h"
#include "SirEngine/animation/skeleton.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"
#include "SirEngine/scripting/scriptingContext.h"

extern "C" {
#include <lua/lua.h>
}

namespace SirEngine {

static const std::string TYPE_KEY = "type";
static const std::string SKELETON_KEY = "skeleton";
static const std::string ASSET_NAME_KEY = "assetName";
static const std::string ANIMATION_CLIPS_KEY = "animationClips";
static const std::string SCRIPT_KEY = "luaScript";
static const std::string START_STATE_KEY = "startState";
static const std::string ANIMATION_CONFIG_NAME_KEY = "name";

LuaStatePlayer::LuaStatePlayer() : AnimationPlayer() {}

LuaStatePlayer::~LuaStatePlayer() {}

void LuaStatePlayer::init(AnimationManager *manager,
                          nlohmann::json &configJson) {
  const std::string empty;
  const std::string configName =
      getValueIfInJson(configJson, ANIMATION_CONFIG_NAME_KEY, empty);
  assert(!configName.empty());

  const std::string assetNameFile =
      getValueIfInJson(configJson, ASSET_NAME_KEY, empty);
  const std::string skeletonFile =
      getValueIfInJson(configJson, SKELETON_KEY, empty);
  // skeleton
  // checking if the skeleton is already cached, if not load it
  const std::string skeletonFileName = getFileName(skeletonFile);

  skeleton =
      manager->loadSkeleton(skeletonFileName.c_str(), skeletonFile.c_str());
  assert(skeleton != nullptr);

  // load the script
  const std::string scriptPath =
      getValueIfInJson(configJson, SCRIPT_KEY, empty);
  assert(!scriptPath.empty());
  stateMachine =
      globals::SCRIPTING_CONTEXT->loadScript(scriptPath.c_str(), true);

  // now we need to iterate the animations and make sure they are loaded, they
  // will reference them by name for now, so no need to store a handle or
  // pointer just make sure they are loaded

  const auto found = configJson.find(ANIMATION_CLIPS_KEY);
  if (found == configJson.end()) {
    assert(0 &&
           "Could not find animations clips in given state machine config");
  }
  for (const auto &anim : found.value()) {
    const auto clipPath = anim.get<std::string>();

    assert(!clipPath.empty());

    // animation
    // checking if the animation clip is already cached, if not load it
    const std::string animationClipFileName = getFileName(clipPath);
    manager->loadAnimationClip(animationClipFileName.c_str(), clipPath.c_str());
  }

  // start state
  const std::string startState =
      getValueIfInJson(configJson, START_STATE_KEY, empty);
  assert(!startState.empty());
  currentState = persistentString(startState.c_str());

  // allocating named pose
  m_outPose = manager->getSkeletonPose(skeleton);
  m_globalStartStamp = manager->getAnimClock().getTicks();
  m_flags = ANIM_FLAGS::READY;
}

inline DirectX::XMFLOAT3 lerp3(const DirectX::XMFLOAT3 &v1,
                               const DirectX::XMFLOAT3 &v2,
                               const float amount) {
  return DirectX::XMFLOAT3{
      ((1.0f - amount) * v1.x) + (amount * v2.x),
      ((1.0f - amount) * v1.y) + (amount * v2.y),
      ((1.0f - amount) * v1.z) + (amount * v2.z),
  };
}
void LuaStatePlayer::evaluate(long long stampNS) {

  const ScriptData &data =
      globals::SCRIPTING_CONTEXT->getScriptDataFromHandle(stateMachine);
  lua_State *state = globals::SCRIPTING_CONTEXT->getContext();
  lua_getglobal(state, "evaluate");
  lua_pushstring(state, currentState);
  const int status = lua_pcall(state, 1, 1, 0);
  if (status != LUA_OK) {
    const char *message = lua_tostring(state, -1);
    SE_CORE_ERROR(message);
    lua_pop(state, 1);
    assert(0);
  }
  int x = 0;
  currentState = lua_tostring(state,-1); 
  /*
  assert(m_clip != nullptr);
  assert(stampNS >= 0);
  float NANO_TO_SECONDS = float(1e-9);

  // we convert to seconds, since we need to count how many frames
  // passed and that is expressed in seconds
  const float speedTimeMultiplier = NANO_TO_SECONDS * m_multiplier;
  const float delta = (stampNS - m_globalStartStamp) * speedTimeMultiplier;
  // dividing the time elapsed since we started playing animation
  // and divide by the frame-rate so we know how many frames we played so far
  const float framesElapsedF = delta / (m_clip->m_frameRate);
  const int framesElapsed = static_cast<int>(floor(framesElapsedF));
  // converting the frames in loop
  const int startIdx = framesElapsed % m_clip->m_frameCount;

  // convert counter to idx
  // we find the two frames we need to interpoalte to
  int endIdx = startIdx + 1;
  const int endRange = m_clip->m_frameCount - 1;
  // if the end frame is out of the range it means needs to loop around
  if (endIdx > endRange) {
    endIdx = 0;
  }

  // extracting the two poses
  const JointPose *startP =
      m_clip->m_poses + (startIdx * m_clip->m_bonesPerFrame);
  const JointPose *endP = m_clip->m_poses + (endIdx * m_clip->m_bonesPerFrame);
  // here we find how much in the frame we are, we do that
  // by subtracting the frames elapsed in float minus the floored
  // value basically leaving us only with the decimal part as
  // it was a modf
  const float interpolationValue = (framesElapsedF - float(framesElapsed));

  //#pragma omp parallel for
  for (unsigned int i = 0; i < m_outPose->m_skeleton->m_jointCount; ++i) {
    // interpolating all the bones in local space
    auto &jointStart = startP[i];
    auto &jointEnd = endP[i];

    // we slerp the rotation and linearlly interpolate the
    // translation
    const DirectX::XMVECTOR rot = DirectX::XMQuaternionSlerp(
        jointStart.m_rot, jointEnd.m_rot, interpolationValue);

    const DirectX::XMFLOAT3 pos =
        lerp3(jointStart.m_trans, jointEnd.m_trans, interpolationValue);
    // the compiler should be able to optimize out this copy
    m_outPose->m_localPose[i].m_rot = rot;
    m_outPose->m_localPose[i].m_trans = pos;
  }
  // now that the anim has been blended I will compute the
  // matrices in world-space (skin ready)
  m_outPose->updateGlobalFromLocal();
  m_flags = ANIM_FLAGS::NEW_MATRICES;
  */
}

uint32_t LuaStatePlayer::getJointCount() const {
  return skeleton->m_jointCount;
}
} // namespace SirEngine
