#pragma once

#include "SirEngine/animation/skeleton.h"

namespace SirEngine {

enum class ANIM_CLIP_KEYWORDS { L_FOOT_DOWN = 1, R_FOOT_DOWN = 2 };

struct AnimationMetadataKey {
  ANIM_CLIP_KEYWORDS m_key;
  int m_value;
};

struct SIR_ENGINE_API AnimationClip {

  AnimationClip() = default;
  ~AnimationClip();
  bool initialize(const char *path);
  int findFirstMetadataFrame(ANIM_CLIP_KEYWORDS flag) const;
  int findMetadataFrameFromGivenFrame(const ANIM_CLIP_KEYWORDS flag,
                                      const int sourceFrame) const;

  JointPose *m_poses = nullptr;
  const char *m_name = nullptr;
  AnimationMetadataKey *m_metadata = nullptr;
  int m_metadataCount = 0;
  int m_frameCount = 0;
  int m_bonesPerFrame = 0;
  float m_frameRate = 1.0f;
  bool m_isLoopable = false;
};

} // namespace SirEngine
