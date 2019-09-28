#pragma once

#include "SirEngine/animation/skeleton.h"
#include <string>

namespace SirEngine {

struct AnimPose {
  JointPose *m_local_pose = nullptr;
  uint32_t size;
};
enum class ANIM_FLAGS { READY = 1, NEW_MATRICES = 2 };

struct AnimationClip {

  AnimationClip() = default;
  ~AnimationClip();
  bool initialize(const char *path);

  JointPose *m_poses;
  const char *m_name;
  int m_frameCount;
  int m_bonesPerFrame;
  float m_frameRate;
  bool m_isLoopable;
};

// this structure represent a state of an animaiton,
// meaning all the data needed to evalaute an animation,
// what the clip is and what the  skeleton is etc
struct AnimState {

  static const float NANO_TO_SECONDS;
  // the name of the clip
  //TODO FIX name here
  std::string name;
  AnimationClip *clip = nullptr;
  SkeletonPose *m_pose = nullptr;
  // speed multiplier for the clip
  float multiplier;
  // wheter or not the animation should loop
  bool m_loop;
  // global time at which the animation started,
  // this imply that I am usinga global clock to perform
  // the calculation, any kind of sync need to be done with the
  // global clock
  long long globalStartStamp;
  ANIM_FLAGS m_flags = ANIM_FLAGS::READY;

  void updateGlobalByAnim(long long stampNS) ;
};
} // namespace SirEngine
