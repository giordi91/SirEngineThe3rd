#pragma once

#include "SirEngine/animation/skeleton.h"

namespace SirEngine {

enum class ANIM_CLIP_KEYWORDS { L_FOOT_DOWN = 1, R_FOOT_DOWN = 2 };
enum class ANIM_FLAGS { READY = 1, NEW_MATRICES = 2 };

struct AnimationMetadataKey {
  int m_key;
  ANIM_CLIP_KEYWORDS m_value;
};

struct SIR_ENGINE_API AnimationClip {

  AnimationClip() = default;
  ~AnimationClip();
  bool initialize(const char *path);
  int findFirstMetadataFrame(ANIM_CLIP_KEYWORDS flag);

  JointPose *m_poses = nullptr;
  const char *m_name = nullptr;
  AnimationMetadataKey *m_metadata = nullptr;
  int m_metadataCount = 0;
  int m_frameCount = 0;
  int m_bonesPerFrame = 0;
  float m_frameRate = 1.0f;
  bool m_isLoopable = false;
};

// this structure represent a state of an animation,
// meaning all the data needed to evalaute an animation,
// what the clip is and what the  skeleton is etc
struct AnimState {

  static const float NANO_TO_SECONDS;
  const char *m_name;
  AnimationClip *m_clip = nullptr;
  SkeletonPose *m_pose = nullptr;
  // speed multiplier for the clip
  float m_multiplier;
  // whether or not the animation should loop
  bool m_loop;
  // global time at which the animation started,
  // this imply that I am using a global clock to perform
  // the calculation, any kind of sync need to be done with the
  // global clock
  long long m_globalStartStamp;
  ANIM_FLAGS m_flags = ANIM_FLAGS::READY;

  void updateGlobalByAnim(long long stampNS);
};

} // namespace SirEngine
