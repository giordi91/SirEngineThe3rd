#pragma once
#include "SirEngine/animation/animationPlayer.h"
#include "SirEngine/handle.h"

// NOTE: not particularly happy with this, luckily is just forward declare but
// still bleeds json type around, the reason why this is here is because either
// we read and or parse the file twice or we pass the actual parsed file, the
// caller needs to get the type out from the json to figure out the right
// factory function to be called, then the callee will get whatever data needs
// from there. One option is to pass a void pointer and cast it internally which
// is not really pretty either
#include "nlohmann/json_fwd.hpp"
#include <queue>
#include "animationClip.h"

namespace SirEngine {
struct SkeletonPose;
struct Skeleton;
class AnimationManager;
struct AnimationClip;

class LuaStatePlayer final : public AnimationPlayer {
  enum class TRANSITION_STATUS { NEW = 0, WAITING_FOR_TRANSITION_FRAME = 1, TRANSITIONING=2,DONE=3};

  struct Transition {
    const char *m_targetAnimation = nullptr;
    const char *m_targetState = nullptr;
    //long long m_destinationOriginalTime = 0;
    long long m_endTransitionTime;
    int m_transitionFrameSrc = 0;
    int m_transitionFrameDest = 0;
    int m_frameOverlap = 4;
    ANIM_CLIP_KEYWORDS m_transitionKeyID;
    TRANSITION_STATUS m_status = TRANSITION_STATUS::NEW;
    long long m_endTransitionRange ;
    long long m_startTransitionTime;
    long long m_destAnimOffset;
  };

  struct AnimationEvalRequest {
    const char *m_animation = nullptr;
    SkeletonPose *m_destination = nullptr;
    long long m_stampNS = 0;
    long long m_originTime = 0;
	bool convertToGlobals = true; 
  };

  struct InterpolateTwoPosesRequest
  {
	  float factor;
	  SkeletonPose* src;
	  SkeletonPose* dest;
	  SkeletonPose* output;
  };

public:
  LuaStatePlayer();
  ~LuaStatePlayer() override;
  void init(AnimationManager *manager, nlohmann::json &configJson);
  void evaluate(long long stampNS) override;
  uint32_t getJointCount() const override;

  //TODO clear up to private stuff
  void evaluateAnim(const AnimationEvalRequest *request);
  int convertTimeToFrames(const long long currentStamp,
                          const long long originStamp,
                          const AnimationClip *clip) const;
private:
  void interpolateTwoPoses(InterpolateTwoPosesRequest& finalRequest);
  void submitInterpRequest(long long timeStamp, Transition* transition, float ratio);
  bool performTransition(Transition *transition, const long long timeStamp);

private:
  Skeleton *skeleton;
  float m_multiplier = 1.0f;
  ScriptHandle stateMachine;
  const char *currentState = "";
  const char *currentAnim = "";
  SkeletonPose *m_transitionSource = nullptr;
  SkeletonPose *m_transitionDest = nullptr;
  // temporary
  std::queue<Transition> m_transitionsQueue;
  Transition *m_currentTransition = nullptr;
};

} // namespace SirEngine
