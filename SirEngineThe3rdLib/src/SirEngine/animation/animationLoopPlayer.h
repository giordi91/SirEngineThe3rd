#pragma once
#include "SirEngine/animation/animationPlayer.h"
#include "nlohmann/json_fwd.hpp"
#include <DirectXMath.h>

namespace SirEngine {
struct SkeletonPose;
struct Skeleton;
class AnimationManager;
struct AnimationClip;


class AnimationLoopPlayer final : public AnimationPlayer {
public:
  AnimationLoopPlayer();
  ~AnimationLoopPlayer() override;
  void init(AnimationManager *manager, nlohmann::json &configJson);
  void evaluate(long long stampNS) override;
  uint32_t getJointCount() const override;
private:
  Skeleton *skeleton;
  AnimationClip *m_clip;
  float m_multiplier = 1.0f;
  DirectX::XMMATRIX m_transform;
};

} // namespace SirEngine
