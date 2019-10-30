#include "SirEngine/animation/animationManipulation.h"
#include "SirEngine/animation/animationClip.h"
#include "animationManager.h"
#include "skeleton.h"

namespace SirEngine {
inline DirectX::XMFLOAT3 lerp3(const DirectX::XMFLOAT3 &v1,
                               const DirectX::XMFLOAT3 &v2,
                               const float amount) {
  return DirectX::XMFLOAT3{
      ((1.0f - amount) * v1.x) + (amount * v2.x),
      ((1.0f - amount) * v1.y) + (amount * v2.y),
      ((1.0f - amount) * v1.z) + (amount * v2.z),
  };
}

void evaluateAnim(const AnimationEvalRequest *request) {
  // need to fetch the clip!
  auto *clip =
      globals::ANIMATION_MANAGER->getAnimationClipByName(request->m_animation);
  // assert(m_clip != nullptr);
  const long long stampNS = request->m_stampNS;
  assert(stampNS >= 0);

  // we convert to seconds, since we need to count how many frames
  // passed and that is expressed in seconds
  const float speedTimeMultiplier =
      TimeConversion::NANO_TO_SECONDS * request->m_multiplier;
  const float delta = (stampNS - request->m_originTime) * speedTimeMultiplier;
  // dividing the time elapsed since we started playing animation
  // and divide by the frame-rate so we know how many frames we played so far
  const float framesElapsedF = delta / (clip->m_frameRate);
  const int framesElapsed = static_cast<int>(floor(framesElapsedF));
  // converting the frames in loop
  const int startIdx = framesElapsed % clip->m_frameCount;

  // convert counter to idx
  // we find the two frames we need to interpolate to
  int endIdx = startIdx + 1;
  const int endRange = clip->m_frameCount - 1;
  // if the end frame is out of the range it means needs to loop around
  if (endIdx > endRange) {
    endIdx = 0;
  }

  // extracting the two poses
  const JointPose *startP = clip->m_poses + (startIdx * clip->m_bonesPerFrame);
  const JointPose *endP = clip->m_poses + (endIdx * clip->m_bonesPerFrame);
  // here we find how much in the frame we are, we do that
  // by subtracting the frames elapsed in float minus the floored
  // value basically leaving us only with the decimal part as
  // it was a modf
  const float interpolationValue = (framesElapsedF - float(framesElapsed));

  //#pragma omp parallel for
  for (unsigned int i = 0; i < request->m_destination->m_skeleton->m_jointCount;
       ++i) {
    // interpolating all the bones in local space
    auto &jointStart = startP[i];
    auto &jointEnd = endP[i];

    // we slerp the rotation and linearly interpolate the
    // translation
    const DirectX::XMVECTOR rot = DirectX::XMQuaternionSlerp(
        jointStart.m_rot, jointEnd.m_rot, interpolationValue);

    const DirectX::XMFLOAT3 pos =
        lerp3(jointStart.m_trans, jointEnd.m_trans, interpolationValue);
    // the compiler should be able to optimize out this copy
    request->m_destination->m_localPose[i].m_rot = rot;
    request->m_destination->m_localPose[i].m_trans = pos;
  }
  // now that the anim has been blended I will compute the
  // matrices in world-space (skin ready)
  request->m_destination->updateGlobalFromLocal(request->m_transform);
}

void interpolateTwoPoses(InterpolateTwoPosesRequest& request)
{
	//#pragma omp parallel for
	for (unsigned int i = 0; i < request.output->m_skeleton->m_jointCount; ++i)
	{
		// interpolating all the bones in local space
		JointPose& jointStart = request.src->m_localPose[i];
		JointPose& jointEnd = request.dest->m_localPose[i];

		// we slerp the rotation and linearly interpolate the
		// translation
		const DirectX::XMVECTOR rot = DirectX::XMQuaternionSlerp(
			jointStart.m_rot, jointEnd.m_rot, request.factor);

		const DirectX::XMFLOAT3 pos =
			lerp3(jointStart.m_trans, jointEnd.m_trans, request.factor);
		// the compiler should be able to optimize out this copy
		request.output->m_localPose[i].m_rot = rot;
		request.output->m_localPose[i].m_trans = pos;
	}
	// now that the anim has been blended I will compute the
	// matrices in world-space (skin ready)
	request.output->updateGlobalFromLocal(request.m_transform);
}
} // namespace SirEngine
