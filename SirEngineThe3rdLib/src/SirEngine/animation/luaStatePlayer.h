#pragma once
#include "SirEngine/animation/animationPlayer.h"

#include "SirEngine/handle.h"

// NOTE: not particularly happy with this, luckily is just forward declare but
// still bleeds json type around, the reason why this is here is because either
// we read and or parse the file twice or we pass the actual parsed file, the
// caller needs to get the type out from the json to figure out the right
// factory function to be called, then the callee will get whatever data needs
// from there. One option is to pass a void pointer and cast it internally which
// is not really pretty either
#include "nlohmann/json_fwd.hpp"
// TODO remove std::queue
#include <queue>
// once the queue is removed I can just also swap the allocation for pointers in
// a pool and not have the include
#include "SirEngine/animation/animationManipulation.h"
#include <DirectXMath.h>

namespace SirEngine {
struct SkeletonPose;
struct Skeleton;
class AnimationManager;

class LuaStatePlayer final : public AnimationPlayer {

public:
  LuaStatePlayer() : AnimationPlayer() {}
  ~LuaStatePlayer() override = default;

  void init(AnimationManager *manager, nlohmann::json &configJson);
  void evaluate(long long stampNS) override;
  uint32_t getJointCount() const override;

private:
  void evaluateStateMachine();
  void submitInterpRequest(long long timeStamp, Transition *transition,
                           float ratio);
  bool performTransition(Transition *transition, const long long timeStamp);

private:
  Skeleton *skeleton = nullptr;
  float m_multiplier = 1.0f;
  ScriptHandle stateMachine{};
  const char *currentState = "";
  const char *currentAnim = "";
  SkeletonPose *m_transitionSource = nullptr;
  SkeletonPose *m_transitionDest = nullptr;
  Transition *m_currentTransition = nullptr;
  int m_queueMaxSize = 2;
  float m_currentCogSpeed =0.0f;
  DirectX::XMMATRIX m_transform;

  // temporary
  std::queue<Transition> m_transitionsQueue;
};

} // namespace SirEngine
