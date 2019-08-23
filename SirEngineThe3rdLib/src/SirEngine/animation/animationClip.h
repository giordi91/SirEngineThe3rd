#pragma once

#include "SirEngine/animation/skeleton.h"
#include <string>
#include <vector>

namespace SirEngine {

struct AnimPose {
  std::vector<JointPose> m_local_pose;
};

struct AnimationClip {

  AnimationClip() = default;
  ~AnimationClip() = default;
  bool initialize(const std::string &path);

  std::vector<AnimPose> m_poses;
  std::string m_name;
  int m_frameCount;
  float m_frameRate;
  bool m_isLoopable;
};

// this structure rapresent a state of an animaiton,
// meaning all the data needed to evalaute an animation,
// what the clip is and what the  skeleton is etc
struct AnimState {

  static const float NANO_TO_SECONDS;
  // the name of the clip
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

  void updateGlobalByAnim(long long stampNS);
};
} // namespace animation
