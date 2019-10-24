#pragma once
#include <stdint.h>

namespace SirEngine {
struct SkeletonPose;
// Abstract interface for something that can evaluate animation,
class AnimationPlayer {
public:
  AnimationPlayer() = default;
  virtual ~AnimationPlayer() = default;

  // pure function used to evaluate the the animation
  virtual void evaluate(long long stampNS) = 0;
  virtual uint32_t getJointCount() const = 0;
  SkeletonPose *getOutPose() const { return outPose; };

protected:
  long long m_globalStartStamp;
  SkeletonPose *outPose;
};

} // namespace SirEngine