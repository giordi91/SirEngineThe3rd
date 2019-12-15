#include "SirEngine/animation/luaStatePlayer.h"
#include "SirEngine/animation/animationClip.h"
#include "SirEngine/animation/animationManager.h"
#include "SirEngine/animation/animationManipulation.h"
#include "SirEngine/animation/skeleton.h"
#include "SirEngine/scripting/scriptingContext.h"

#include "SirEngine/fileUtils.h"
#include "SirEngine/input.h"
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
static const std::string ANIMATION_CONFIG_NAME_KEY = "name";
static const char *EVALUATE_FUNCTION_NAME = "evaluate";
static const char *START_FUNCTION_NAME = "start";

int convertTimeToFrames(const int64_t currentStamp, const int64_t originStamp,
                        const AnimationClip *clip, float speedMultiplier) {
  // we convert to seconds, since we need to count how many frames
  // passed and that is expressed in seconds
  const double speedTimeMultiplier =
      TimeConversion::NANO_TO_SECONDS * static_cast<double>(speedMultiplier);
  const double delta = (currentStamp - originStamp) * speedTimeMultiplier;
  // dividing the time elapsed since we started playing animation
  // and divide by the frame-rate so we know how many frames we played so far
  const double framesElapsedF = delta / (clip->m_frameRate);
  const int framesElapsed = static_cast<int>(floor(framesElapsedF));
  // converting the frames in loop
  const int frame = framesElapsed % clip->m_frameCount;
  return frame;
}

void LuaStatePlayer::init(AnimationManager *manager,
                          nlohmann::json &configJson) {
  m_transform = glm::mat4(1.0f);
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

  // allocating named pose
  m_outPose = manager->getSkeletonPose(skeleton);
  // scratch memory to perform the transition
  m_transitionSource = manager->getSkeletonPose(skeleton);
  m_transitionDest = manager->getSkeletonPose(skeleton);
  m_startTimeStamp = manager->getAnimClock().getTicks();
  m_flags = ANIM_FLAGS::READY;

  // execute the start of the state machine
  lua_State *state = globals::SCRIPTING_CONTEXT->getContext();
  lua_getglobal(state, START_FUNCTION_NAME);

  const int status = lua_pcall(state, 0, 3, 0);
  if (status != LUA_OK) {
    const char *message = lua_tostring(state, -1);
    SE_CORE_ERROR(message);
    lua_pop(state, 1);
    assert(0);
  }

  const char *newState = lua_tostring(state, -3);
  assert(newState != nullptr);
  currentState = persistentString(newState);
  const char *currentAnimStr = lua_tostring(state, -2);
  assert(currentAnimStr != nullptr);
  currentAnim = persistentString(currentAnimStr);
  auto cogSpeed = static_cast<float>(lua_tonumber(state, -1));

  m_currentCogSpeed = cogSpeed;
}

void LuaStatePlayer::evaluateStateMachine() {

  // execute the lua state machine
  lua_State *state = globals::SCRIPTING_CONTEXT->getContext();
  lua_getglobal(state, EVALUATE_FUNCTION_NAME);
  lua_pushstring(state, currentState);
  const int status = lua_pcall(state, 1, 6, 0);
  if (status != LUA_OK) {
    const char *message = lua_tostring(state, -1);
    SE_CORE_ERROR(message);
    lua_pop(state, 1);
    assert(0);
  }
  // TODO might be worth to return the table directly and parse that
  // the first returned argument is the state that the state machine decided to
  // be in
  const char *newState = lua_tostring(state, -6);
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
    // currently not used
    // const char *sourceAnim = lua_tostring(state, -5);
    const char *targetAnim = lua_tostring(state, -4);
    const char *transitionKey = lua_tostring(state, -3);
    auto transitionLenInSeconds = static_cast<float>(lua_tonumber(state, -2));
    auto cogSpeed = static_cast<float>(lua_tonumber(state, -1));

    // stringFree(currentState);
    // currentState = persistentString(newState);
    Transition transition;
    transition.m_targetAnimation = persistentString(targetAnim);
    transition.m_targetState = persistentString(newState);
    transition.m_transitionLength = transitionLenInSeconds;
    transition.m_cogSpeed = cogSpeed;

    // lets convert the transition key from string to the correct enum
    if (strlen(transitionKey) != 0) {
      transition.m_transitionKeyID = static_cast<ANIM_CLIP_KEYWORDS>(
          globals::ANIMATION_MANAGER->animationKeywordNameToValue(
              transitionKey));
    } else {
      transition.m_transitionKeyID = ANIM_CLIP_KEYWORDS::NONE;
    }

    // adding the transition to the queue for later evaluation
    m_transitionsQueue.push(transition);

    // we let current state move forward such that state machine evaluates
    // correctly, but we throttle the evaluation based on the max state changes
    // queue
    stringFree(currentState);
    currentState = persistentString(newState);
  }
  lua_pop(state, 6);
}

void LuaStatePlayer::updateTransform() {
  // buttons
  int leftArrow = 37;
  int rightArrow = 39;
  float speed = 0.001f;
  bool leftArrowDown = globals::INPUT->isKeyDown(leftArrow);
  bool rightArrowDown = globals::INPUT->isKeyDown(rightArrow);
  float spinValue = leftArrowDown ? -speed : 0.0f;
  spinValue = rightArrowDown ? speed + spinValue : spinValue;

  // move back transform
  auto pos = m_transform[3];
  auto translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-pos));

  auto toSpin = translationMatrix * m_transform;

  // spin
  auto spin = glm::rotate(glm::mat4(1.0f), spinValue, glm::vec3(0, 1, 0));
  m_transform = spin * toSpin;

  // now translate using cog speed
  auto forward = m_transform[2];
  auto offsetVector = forward * m_workingCogSpeed;
  translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(offsetVector));
  m_transform = translationMatrix * m_transform;
  // add back the offset
  translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(pos));
  m_transform = translationMatrix * m_transform;
}

void LuaStatePlayer::evaluate(const int64_t stampNS) {

  updateTransform();

  // if the queue is too full we just prevent the state machine from evaluating
  bool shouldEvaluate = m_transitionsQueue.size() < m_queueMaxSize;
  if (shouldEvaluate) {
    evaluateStateMachine();
  }

  // let us check if there is any transition to be done
  if (m_currentTransition == nullptr && m_transitionsQueue.empty()) {

    // no transition to make, let us perform a simple animation evaluation
    AnimationEvalRequest eval{currentAnim,      m_outPose,    stampNS,
                              m_startTimeStamp, m_multiplier, true,
                              m_transform};
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
      m_startTimeStamp = m_currentTransition->m_destAnimStartTimeStamp;
      m_currentCogSpeed = m_currentTransition->m_cogSpeed;
      m_workingCogSpeed = m_currentCogSpeed;
      ;

      // cleaning up the transition
      m_currentTransition = nullptr;
      m_transitionsQueue.pop();
    }
  }
}

bool LuaStatePlayer::performTransition(Transition *transition,
                                       const int64_t timeStamp) {

  // let us first fetch the clip from the animation manager, I am not
  // particularly happy about this fetch based on string, BUT I will worry
  // once the profile shows is an actual problem for now this will do
  const AnimationClip *clip =
      globals::ANIMATION_MANAGER->getAnimationClipByName(currentAnim);
  const AnimationClip *clipDest =
      globals::ANIMATION_MANAGER->getAnimationClipByName(
          transition->m_targetAnimation);

  const double frameLenInMS = clip->m_frameRate * 1000.0;

  if (transition->m_status == TRANSITION_STATUS::NEW) {

    // we have a brand new transition, there are few parameters that needs to be
    // computed
    // first figure out at which frame we are going to transition the current
    // clip
    const int currentFrame =
        convertTimeToFrames(timeStamp, m_startTimeStamp, clip, m_multiplier);
    transition->m_transitionFrameSrc = clip->findMetadataFrameFromGivenFrame(
        transition->m_transitionKeyID, currentFrame);
    assert(transition->m_transitionFrameSrc != -1);

    // getting at which frame we need to transition the destination
    transition->m_transitionFrameDest =
        clipDest->findFirstMetadataFrame(transition->m_transitionKeyID);
    assert(transition->m_transitionFrameDest != -1);

    // so we know which frame we are at, and at which frame the transition will
    // need to happen we can compute an offset in nanoseconds
    int deltaFrames =
        transition->m_transitionFrameSrc -
        currentFrame; // delta from the frame we are currently at and the frame
                      // the main animation will start the transition

    // next we compute the time stamp at which the transition will start to
    // evaluate
    const auto deltaFramesInNano = static_cast<int64_t>(
        deltaFrames * frameLenInMS * TimeConversion::MS_TO_NANO);
    int64_t timeStampTransitionStart = timeStamp + deltaFramesInNano;

    // now that we know when we start the transition, we compute
    // how long the transition is to find the time stamp at which the
    // transition will end
    const auto transitionDurationInNano = static_cast<int64_t>(
        static_cast<double>(transition->m_transitionLength) *
        TimeConversion::SECONDS_TO_NANO);
    int64_t timeStampTransitionEnd =
        timeStampTransitionStart + transitionDurationInNano;

    // now we can compute what offset is needed to align the second animation
    // such that it was started at the right time such that at the end of the
    // transition we should play the right frame
    const int64_t offsetDest =
        static_cast<int64_t>(double(transition->m_transitionFrameDest) *
                             frameLenInMS * TimeConversion::MS_TO_NANO);
    int64_t stampDestinationStart = timeStampTransitionEnd - offsetDest;

    // there can be the case where the transition is longer than the amount of
    // animation before the start frame in the destination. For example
    // transition is 20 frames, but the transition frame in the destination is
    // 5, there are only 5 frames left, if we shift the start time of the the
    // animation by 5 frames only we would get a negative time delta, because
    // our destination animation did not start yet, so what we do is simply
    // subtract enough full animations length in nanoseconds such that we have
    // an actual start time before our current time stamp. Since we are removing
    // multiple of the whole animation it won't alter the actual local offset
    if ((timeStamp - stampDestinationStart) < 0) {
      // computing time delta inbetween our current time and where animation
      // should start
      // playing
      const int64_t absDelta = abs(timeStamp - stampDestinationStart);
      // we compute the total length in nanoseconds
      const int64_t destLenInNano =
          static_cast<int64_t>(static_cast<double>(clipDest->m_frameCount) *
                               frameLenInMS * TimeConversion::MS_TO_NANO);
      // we perform the division and floor it, and add one such that we are 100%
      // sure that we are going to subtract enough
      const int64_t deltaCount = (absDelta / destLenInNano) + 1;
      // perform the shift
      stampDestinationStart -= destLenInNano * deltaCount;
    }
    assert((timeStamp - stampDestinationStart) > 0);

    transition->m_startTransitionTime = timeStampTransitionStart;
    transition->m_endTransitionTime = timeStampTransitionEnd;
    transition->m_destAnimStartTimeStamp = stampDestinationStart;
    transition->m_status = TRANSITION_STATUS::TRANSITIONING;
  }

  // first we check whether or not we should play a transition, if not,
  // it means we are waiting for our designated frame and we just keep
  // playing current animation
  if (timeStamp >= transition->m_startTransitionTime) {

    // lets compute the interpoaltion factor for the two animations
    const auto range = static_cast<double>(transition->m_endTransitionTime -
                                           transition->m_startTransitionTime);
    assert(range > 0.0);
    const auto ratio = static_cast<float>(
        static_cast<double>(timeStamp - transition->m_startTransitionTime) /
        range);

    // making sure the ratio is positive, baking in a small delta just to be
    // on the safe side for float comparison
    assert(ratio > -0.00001f);
    if (ratio >= 1.0f) {
      // we are past the range, no need o interpolate
      AnimationEvalRequest eval{transition->m_targetAnimation,
                                m_outPose,
                                timeStamp,
                                m_startTimeStamp,
                                m_multiplier,
                                true,
                                m_transform};
      evaluateAnim(&eval);
      transition->m_status = TRANSITION_STATUS::DONE;
      // we are done with the transition return true
      return true;

    } else {
      submitInterpRequest(timeStamp, transition, ratio);
    }
  } else {
    AnimationEvalRequest eval{currentAnim,      m_outPose,    timeStamp,
                              m_startTimeStamp, m_multiplier, true,
                              m_transform};
    evaluateAnim(&eval);
  }
  m_flags = ANIM_FLAGS::NEW_MATRICES;
  return false;
}

void LuaStatePlayer::submitInterpRequest(const int64_t timeStamp,
                                         Transition *transition,
                                         const float ratio) {
  // we have to make two interpolation requests
  // source interpolation request request
  AnimationEvalRequest srcRequest{};
  srcRequest.convertToGlobals = false;
  srcRequest.m_animation = currentAnim;
  srcRequest.m_destination = m_transitionSource;
  srcRequest.m_stampNS = timeStamp;
  srcRequest.m_originTime = m_startTimeStamp;
  srcRequest.m_multiplier = 1.0f;
  srcRequest.m_transform =
      m_transform; // not used since convert to global is off
  evaluateAnim(&srcRequest);

  // now we interpolate the destination
  AnimationEvalRequest destRequest{};
  destRequest.convertToGlobals = false;
  destRequest.m_animation = transition->m_targetAnimation;
  destRequest.m_destination = m_transitionDest;
  destRequest.m_stampNS = timeStamp;
  destRequest.m_originTime = transition->m_destAnimStartTimeStamp;
  destRequest.m_multiplier = 1.0f;
  destRequest.m_transform =
      m_transform; // not used since convert to global is off
  evaluateAnim(&destRequest);

  // now we need to interpolate not two frames but to existing poses
  // one from the source and one from the destination animation
  InterpolateTwoPosesRequest finalRequest{};
  finalRequest.factor = ratio;
  finalRequest.src = m_transitionSource;
  finalRequest.dest = m_transitionDest;
  finalRequest.output = m_outPose;
  finalRequest.m_transform = m_transform;

  interpolateTwoPoses(finalRequest);
  m_flags = ANIM_FLAGS::NEW_MATRICES;

  // interpolate cog speed
  m_workingCogSpeed =
      ratio * transition->m_cogSpeed + (1.0f - 0.0f) * m_currentCogSpeed;
}

uint32_t LuaStatePlayer::getJointCount() const {
  return skeleton->m_jointCount;
}

} // namespace SirEngine
