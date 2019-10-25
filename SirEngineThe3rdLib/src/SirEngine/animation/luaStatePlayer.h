#pragma once
#include "SirEngine/animation/animationPlayer.h"
#include "nlohmann/json_fwd.hpp"

namespace SirEngine {
struct SkeletonPose;
struct Skeleton;
class AnimationManager;
struct AnimationClip;


class LuaStatePlayer final : public AnimationPlayer {
public:
  LuaStatePlayer();
  ~LuaStatePlayer() override;
  void init(AnimationManager *manager, nlohmann::json &configJson);
  void evaluate(long long stampNS) override;
  uint32_t getJointCount() const override;

private:
  Skeleton *skeleton;
  AnimationClip *m_clip;
  float m_multiplier = 1.0f;
};

} // namespace SirEngine
