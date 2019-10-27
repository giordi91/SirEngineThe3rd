#pragma once
#include "SirEngine/animation/animationPlayer.h"
#include "SirEngine/handle.h"

// NOTE: not particularly happy with this, luckily is just forward declare but
// still bleeds json type around, the reason why this is here is because either
// we read and or parse the file twice or we pass the actual parsed file, the
// caller needs to get the type out from the json to figure out the right factory
// function to be called, then the callee will get whatever data needs from
// there. One option is to pass a void pointer and cast it internally which is
// not really pretty either
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

  void evaluateAnim(const char *animation, long long stampNS); 

private:
  Skeleton *skeleton;
  float m_multiplier = 1.0f;
  ScriptHandle stateMachine;
  const char* currentState ="";
  const char* currentAnim="";
};

} // namespace SirEngine
