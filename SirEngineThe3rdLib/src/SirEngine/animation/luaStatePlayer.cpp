#include "SirEngine/animation/luaStatePlayer.h"
#include "SirEngine/animation/animationClip.h"
#include "SirEngine/animation/animationManipulation.h"
#include "SirEngine/animation/animationManager.h"
#include "SirEngine/animation/skeleton.h"
#include "SirEngine/scripting/scriptingContext.h"

#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"

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

int convertTimeToFrames(const long long currentStamp,
                        const long long originStamp, const AnimationClip *clip,
                        float speedMultiplier) {
  // we convert to seconds, since we need to count how many frames
  // passed and that is expressed in seconds
  const float speedTimeMultiplier =
      TimeConversion::NANO_TO_SECONDS * speedMultiplier;
  const float delta = (currentStamp - originStamp) * speedTimeMultiplier;
  // dividing the time elapsed since we started playing animation
  // and divide by the frame-rate so we know how many frames we played so far
  const float framesElapsedF = delta / (clip->m_frameRate);
  const int framesElapsed = static_cast<int>(floor(framesElapsedF));
  // converting the frames in loop
  const int frame = framesElapsed % clip->m_frameCount;
  return frame;
}

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
  // const std::string startState =
  //    getValueIfInJson(configJson, START_STATE_KEY, empty);
  // assert(!startState.empty());
  // currentState = persistentString(startState.c_str());

  // allocating named pose
  m_outPose = manager->getSkeletonPose(skeleton);
  // scratch memory to perform the transition
  m_transitionSource = manager->getSkeletonPose(skeleton);
  m_transitionDest = manager->getSkeletonPose(skeleton);
  m_globalStartStamp = manager->getAnimClock().getTicks();
  m_flags = ANIM_FLAGS::READY;

  // execute the start of the state machine
  const ScriptData &data =
      globals::SCRIPTING_CONTEXT->getScriptDataFromHandle(stateMachine);
  lua_State *state = globals::SCRIPTING_CONTEXT->getContext();
  lua_getglobal(state, "start");

  const int status = lua_pcall(state, 0, 2, 0);
  if (status != LUA_OK) {
    const char *message = lua_tostring(state, -1);
    SE_CORE_ERROR(message);
    lua_pop(state, 1);
    assert(0);
  }
  int x = 0;
  const char *newState = lua_tostring(state, -2);
  assert(newState != nullptr);
  currentState = persistentString(newState);
  const char *currentAnimStr = lua_tostring(state, -1);
  assert(currentAnimStr != nullptr);
  currentAnim = persistentString(currentAnimStr);
}

void LuaStatePlayer::evaluate(long long stampNS) {

  const ScriptData &data =
      globals::SCRIPTING_CONTEXT->getScriptDataFromHandle(stateMachine);
  lua_State *state = globals::SCRIPTING_CONTEXT->getContext();
  lua_getglobal(state, "evaluate");
  lua_pushstring(state, currentState);
  const int status = lua_pcall(state, 1, 4, 0);
  if (status != LUA_OK) {
    const char *message = lua_tostring(state, -1);
    SE_CORE_ERROR(message);
    lua_pop(state, 1);
    assert(0);
  }
  const char *newState = lua_tostring(state, -4);
  // the method among other param returns a state, if the state changed from the
  // one we passed in, it means we also got enough data to perform a transition,
  // (the other arguments
  // return args are:
  //- new state, string: the state the state machine is in
  //- current animation clip: string,
  //- transition to animation clip, string
  //- transition key: string, which key we want to use for the transition
  bool shouldParseArguments = strcmp(newState, currentState) != 0;
  if (shouldParseArguments) {
    // also mean we took a transition to a new state
    const char *sourceAnim = lua_tostring(state, -3);
    const char *targetAnim = lua_tostring(state, -2);
    const char *transitionKey = lua_tostring(state, -1);

    // stringFree(currentState);
    // currentState = persistentString(newState);
    Transition transition;
    transition.m_targetAnimation = persistentString(targetAnim);
    transition.m_targetState = persistentString(newState);
    // lets convert the transition key
    if (strlen(transitionKey) != 0) {
      transition.m_transitionKeyID = static_cast<ANIM_CLIP_KEYWORDS>(
          globals::ANIMATION_MANAGER->animationKeywordNameToValue(
              transitionKey));
    } else {
      transition.m_transitionKeyID = ANIM_CLIP_KEYWORDS::NONE;
    }
    // now that we have the transition key we need to find which frame we are
    // playing to figure out where we are going to transition our current
    // animation
    const AnimationClip *clip =
        globals::ANIMATION_MANAGER->getAnimationClipByName(currentAnim);
    m_transitionsQueue.push(transition);

    // SE_CORE_INFO("New target state: {0},{1},{2},{3}", newState, sourceAnim,
    //            targetAnim, transitionKey);
  }

  // let us check if there is any transition to be done
  // lets evaluate the animation
  if (m_currentTransition == nullptr && m_transitionsQueue.empty()) {

    // no transition to make
    AnimationEvalRequest eval{currentAnim, m_outPose, stampNS,
                              m_globalStartStamp};
    // SE_CORE_INFO(stampNS);
    evaluateAnim(&eval);
    m_flags = ANIM_FLAGS::NEW_MATRICES;
  } else {
    // we do indeed need to perform a transition

    // if there is not a currently active transition we get the front of the
    // queue
    if (m_currentTransition == nullptr) {
      m_currentTransition = &m_transitionsQueue.front();
    }

    // now we have a current transition and we need to get started
    const bool completedTransition =
        performTransition(m_currentTransition, stampNS);
    if (completedTransition) {

      currentAnim = m_currentTransition->m_targetAnimation;
      m_globalStartStamp = m_currentTransition->m_destAnimOffset;
      currentState = m_currentTransition->m_targetState;
      m_currentTransition = nullptr;
      m_transitionsQueue.pop();
      std::cout << "time taken " << double(stampNS - tempStart) * 1e-9
                << std::endl;
    }
  }

  tempStart = stampNS;
}

bool LuaStatePlayer::performTransition(Transition *transition,
                                       const long long timeStamp) {
  // first lets check the state of the transition, based on that we will perform
  // different actions
  const AnimationClip *clip =
      globals::ANIMATION_MANAGER->getAnimationClipByName(currentAnim);
  const AnimationClip *clipDest =
      globals::ANIMATION_MANAGER->getAnimationClipByName(
          transition->m_targetAnimation);
  const double framerate = clip->m_frameRate * 1000.0f;

  if (transition->m_status == TRANSITION_STATUS::NEW) {
    tempStart = timeStamp;
    // well well well, brand new transition! better get started then!
    // first figure out at which frame we are going to transition the main clip
    const int currentFrame =
        convertTimeToFrames(timeStamp, m_globalStartStamp, clip, m_multiplier);
    transition->m_transitionFrameSrc = clip->findMetadataFrameFromGivenFrame(
        transition->m_transitionKeyID, currentFrame);
    assert(transition->m_transitionFrameSrc != -1);

    // getting at which frame we need to transition the destination
    transition->m_transitionFrameDest =
        clipDest->findFirstMetadataFrame(transition->m_transitionKeyID);
    assert(transition->m_transitionFrameDest != -1);

    // so we know which frame we are at, and at which frame the transition will
    // need to happen we can compute an offset in nanoseconds
    int deltaFrames = transition->m_transitionFrameSrc - currentFrame;
    long long nanoOffsetForTransition =
        deltaFrames * framerate * TimeConversion::MS_TO_NANO;
    long long timeStampTransitionStart = timeStamp + nanoOffsetForTransition;

    const long long transitionDurationInNano =
        // double(transition->m_frameOverlap) * framerate * MS_TO_NANO;
        double(10) * framerate * TimeConversion::MS_TO_NANO;
    long long timeStampTransitionEnd =
        timeStampTransitionStart + transitionDurationInNano;

    // now we can compute what offset is needed to align the second animation
    // such that it was started at the right time such that at the end of the
    // transition we should play the right frame
    const long long offsetDest = double(transition->m_transitionFrameDest) *
                                 framerate * TimeConversion::MS_TO_NANO;
    long long stampDestinationStart = timeStampTransitionEnd - offsetDest;
    if ((timeStamp - stampDestinationStart) < 0) {
      long long absDelta = abs(timeStamp - stampDestinationStart);
      long long destLenInNano = double(clipDest->m_frameCount) * framerate *
                                TimeConversion::MS_TO_NANO;
      long long deltaCount = floor(absDelta / destLenInNano) + 1;
      stampDestinationStart -= destLenInNano * deltaCount;
    }
    assert((timeStamp - stampDestinationStart) > 0);

    transition->m_startTransitionTime = timeStampTransitionStart;
    transition->m_endTransitionTime = timeStampTransitionEnd;
    transition->m_destAnimOffset = stampDestinationStart;
    transition->m_status = TRANSITION_STATUS::TRANSITIONING;
  }

  const double range =
      transition->m_endTransitionTime - transition->m_startTransitionTime;
  assert(range > 0.0);
  const auto ratio = static_cast<float>(
      (timeStamp - transition->m_startTransitionTime) / range);

  if (timeStamp >= transition->m_startTransitionTime) {
    assert(ratio > -0.00001f);
    if (ratio >= 1.0f) {
      // we are past the range, no need o interpolate
      AnimationEvalRequest eval{transition->m_targetAnimation, m_outPose,
                                timeStamp, m_globalStartStamp};
      evaluateAnim(&eval);
      m_flags = ANIM_FLAGS::NEW_MATRICES;
      transition->m_status = TRANSITION_STATUS::DONE;
      // we are done with the transition return true
      return true;

    } else {
      submitInterpRequest(timeStamp, transition, ratio);
    }
  } else {
    AnimationEvalRequest eval{currentAnim, m_outPose, timeStamp,
                              m_globalStartStamp};
    evaluateAnim(&eval);
    m_flags = ANIM_FLAGS::NEW_MATRICES;
  }
  return false;
}

void LuaStatePlayer::submitInterpRequest(const long long timeStamp,
                                         Transition *transition, const float ratio) {
  // we have to make two interpolation requests
  // source interp request
  AnimationEvalRequest srcRequest{};
  srcRequest.convertToGlobals = false;
  srcRequest.m_animation = currentAnim;
  srcRequest.m_destination = m_transitionSource;
  srcRequest.m_stampNS = timeStamp;
  srcRequest.m_originTime = m_globalStartStamp;
  srcRequest.m_multiplier = 1.0f;
  evaluateAnim(&srcRequest);

  // now we interpolate the destination
  AnimationEvalRequest destRequest{};
  destRequest.convertToGlobals = false;
  destRequest.m_animation = transition->m_targetAnimation;
  destRequest.m_destination = m_transitionDest;
  destRequest.m_stampNS = timeStamp;
  destRequest.m_originTime = transition->m_destAnimOffset;
  destRequest.m_multiplier = 1.0f;
  evaluateAnim(&destRequest);

  // now we need to interpolate not two frames but to existing poses
  InterpolateTwoPosesRequest finalRequest{};
  finalRequest.factor = ratio;
  finalRequest.src = m_transitionSource;
  finalRequest.dest = m_transitionDest;
  finalRequest.output = m_outPose;

  interpolateTwoPoses(finalRequest);
  m_flags = ANIM_FLAGS::NEW_MATRICES;
}

uint32_t LuaStatePlayer::getJointCount() const {
  return skeleton->m_jointCount;
}
} // namespace SirEngine
