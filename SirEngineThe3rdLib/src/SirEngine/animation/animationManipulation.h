#pragma once

namespace SirEngine {

// forward declares
struct SkeletonPose;

enum class ANIM_CLIP_KEYWORDS { L_FOOT_DOWN = 1, R_FOOT_DOWN = 2, NONE };

namespace TimeConversion {
static constexpr long long MS_TO_NANO = 1000000;
static constexpr float NANO_TO_SECONDS = float(1e-9);
} // namespace TimeConversion

struct AnimationMetadataKey {
  ANIM_CLIP_KEYWORDS m_key;
  int m_value;
};

enum class TRANSITION_STATUS { NEW, TRANSITIONING, DONE };

struct Transition {
  const char *m_targetAnimation = nullptr;
  const char *m_targetState = nullptr;
  // long long m_destinationOriginalTime = 0;
  int m_transitionFrameSrc = 0;
  int m_transitionFrameDest = 0;
  int m_frameOverlap = 4;
  ANIM_CLIP_KEYWORDS m_transitionKeyID;
  TRANSITION_STATUS m_status = TRANSITION_STATUS::NEW;
  long long m_startTransitionTime;
  long long m_endTransitionTime;
  long long m_destAnimOffset;
};

struct AnimationEvalRequest {
  const char *m_animation = nullptr;
  SkeletonPose *m_destination = nullptr;
  long long m_stampNS = 0;
  long long m_originTime = 0;
  float m_multiplier = 1.0f;
  bool convertToGlobals = true;
};

struct InterpolateTwoPosesRequest {
  float factor;
  SkeletonPose *src;
  SkeletonPose *dest;
  SkeletonPose *output;
};

struct AnimationEvalRequest;
struct InterpolateTwoPosesRequest;

void evaluateAnim(const AnimationEvalRequest* request);

void interpolateTwoPoses(InterpolateTwoPosesRequest& request);
} // namespace SirEngine