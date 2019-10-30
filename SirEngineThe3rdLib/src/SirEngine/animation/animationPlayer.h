#pragma once
#include <stdint.h>

namespace SirEngine {

enum class ANIM_FLAGS { READY = 1, NEW_MATRICES = 2 };

struct SkeletonPose;
// Abstract interface for something that can evaluate animation,
class AnimationPlayer {
public:
  AnimationPlayer() = default;
  virtual ~AnimationPlayer() = default;

  // pure function used to evaluate the the animation
  virtual void evaluate(long long stampNS) = 0;

  // getters/setters
  virtual uint32_t getJointCount() const = 0;
  [[nodiscard]] SkeletonPose *getOutPose() const { return m_outPose; };
  inline ANIM_FLAGS getFlags() const { return m_flags; }
  inline void setFlags(const ANIM_FLAGS flag) { m_flags = flag; }

protected:
  long long m_startTimeStamp; // high resolution time stamp of when the
                              // animation started playing
  SkeletonPose *m_outPose =
      nullptr; // where the evluated matrices will be stored
  ANIM_FLAGS m_flags;
};

} // namespace SirEngine