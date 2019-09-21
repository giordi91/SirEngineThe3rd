#include "SirEngine/animation/animationClip.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/runtimeString.h"

namespace SirEngine {

// initializing the constants
const float AnimState::NANO_TO_SECONDS = float(1e-9);

AnimationClip::~AnimationClip() {
  globals::PERSISTENT_ALLOCATOR->free(m_poses);
}

bool AnimationClip::initialize(const char *path) {
  // load the binary file
  SE_CORE_INFO("Loading animation clip {0}", path);
  const bool res = fileExists(path);
  assert(res);

  uint32_t fileSize;
  const char *binaryData = frameFileLoad(path, fileSize);

  const auto mapper = getMapperData<ClipMapperData>(binaryData);
  m_isLoopable = mapper->isLoopable;
  m_bonesPerFrame = mapper->bonesPerFrame;
  m_frameCount = mapper->frameCount;
  m_frameRate = mapper->frameRate;

  m_name = persistentString(binaryData + sizeof(BinaryFileHeader));
  m_poses = reinterpret_cast<JointPose *>(
      globals::PERSISTENT_ALLOCATOR->allocate(mapper->posesSizeInByte));
  memcpy(m_poses,
         binaryData + sizeof(BinaryFileHeader) + mapper->nameSizeInByte,
         mapper->posesSizeInByte);
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
void AnimState::updateGlobalByAnim(const long long stampNS) {
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
  m_flags = ANIM_FLAGS::NEW_MATRICES;
}

} // namespace SirEngine
