#include "SirEngine/animation/animationClip.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/runtimeString.h"

namespace SirEngine {

// initializing the constants
const float AnimState::NANO_TO_SECONDS = float(1e-9);
bool AnimationClip::initialize(const char *path) {

  auto jObj = getJsonObj(path);
  // we first check the name because if the name is in the cache
  // we just get out
  const auto name = jObj["name"].get<std::string>();

  // quering the basic data
  m_isLoopable = jObj["looping"].get<bool>();

  // this are the maya start and end frame, not used in the engine, leaving
  // them here as reference
  // int start = j_obj["start"].get<int>();
  // int end = j_obj["end"].get<int>();
  m_frameRate = jObj["frame_rate"].get<float>();

  const int posesSize = int(jObj["poses"].size());
  m_bonesPerFrame = int(jObj["bonesPerPose"]);
  m_poses =
      reinterpret_cast<JointPose *>(globals::PERSISTANT_ALLOCATOR->allocate(
          posesSize * m_bonesPerFrame * sizeof(JointPose)));
  m_frameCount = posesSize;
  m_name = persistentString(name.c_str());

  const DirectX::XMFLOAT3 zeroV(0, 0, 0);
  const DirectX::XMVECTOR zeroQ = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

  int poseCounter = 0;
  for (auto &pose : jObj["poses"]) {
    const int jointSize = int(pose.size());
    assert(jointSize == m_bonesPerFrame);

    auto *joints = m_poses + (poseCounter * jointSize);

    int jointCounter = 0;
    for (auto &joint : pose) {

      const DirectX::XMFLOAT3 position =
          getValueIfInJson<DirectX::XMFLOAT3>(joint, "pos", zeroV);

      // extracting joint quaternion
      // to note I export quaternion as x,y,z,w,  is initialized as
      // w,x,y,z
      const DirectX::XMVECTOR rotation =
          getValueIfInJson<DirectX::XMVECTOR>(joint, "quat", zeroQ);

      joints[jointCounter].m_rot = rotation;
      joints[jointCounter].m_trans = position;

      // scale hardcoded, not used for the time being
      joints[jointCounter].m_scale = 1.0f;

      ++jointCounter;
    }

    ++poseCounter;
  }

  return true;
}


inline DirectX::XMFLOAT3 lerp3(const DirectX::XMFLOAT3 &v1,
                               const DirectX::XMFLOAT3 &v2,
                               const float amount) {
  return DirectX::XMFLOAT3{
      ((1.0f - amount) * v1.x) + (amount * v2.x),
      ((1.0f - amount) * v1.y) + (amount * v2.y),
      ((1.0f - amount) * v1.z) + (amount * v2.z),
  };
}
void AnimState::updateGlobalByAnim(const long long stampNS) const {
  assert(clip != nullptr);
  assert(stampNS >= 0);
  const int globalSize = m_pose->m_globalPose.size();
  const int localSize = m_pose->m_localPose.size();
  assert((globalSize == localSize));

  // we convert to seconds, since we need to count how many frames
  // passed and that is expressed in seconds
  const float speedTimeMultiplier = NANO_TO_SECONDS * multiplier;
  const float delta = (stampNS - globalStartStamp) * speedTimeMultiplier;
  // dividing the time elapsed since we started playing animation
  // and divide by the frame-rate so we know how many frames we played so far
  const float framesElapsedF = delta / (clip->m_frameRate);
  const int framesElapsed = static_cast<int>(floor(framesElapsedF));
  // converting the frames in loop
  const int startIdx = framesElapsed % clip->m_frameCount;

  // convert counter to idx
  // we find the two frames we need to interpoalte to
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
  // by subtracting the frames elapsed in flaot minues the floored
  // value basically leaving us only with the decimal part as
  // it was a modf
  const float interpolationValue = (framesElapsedF - float(framesElapsed));

  //#pragma omp parallel for
  for (unsigned int i = 0; i < m_pose->m_skeleton->m_jointCount; ++i) {
    // interpolating all the bones in local space
    auto &jointStart = startP[i];
    auto &jointEnd = endP[i];

    // we slerp the rotation and linearlly interpolate the
    // translation
    const DirectX::XMVECTOR rot = DirectX::XMQuaternionSlerp(
        jointStart.m_rot, jointEnd.m_rot, interpolationValue);

    const DirectX::XMFLOAT3 pos =
        lerp3(jointStart.m_trans, jointEnd.m_trans, interpolationValue);
    // the compiler should be able to optimize out this copy
    m_pose->m_localPose[i].m_rot = rot;
    m_pose->m_localPose[i].m_trans = pos;
  }
  // now that the anim has been blended I will compute the
  // matrices in world-space (skin ready)
  m_pose->updateGlobalFromLocal();
}

} // namespace SirEngine
