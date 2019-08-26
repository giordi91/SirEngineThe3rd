#include "SirEngine/animation/animationClip.h"
#include "SirEngine/fileUtils.h"

namespace SirEngine{

// initializing the constants
const float AnimState::NANO_TO_SECONDS = float(1e-9);
bool AnimationClip::initialize(const std::string &path) {

  auto j_obj = getJsonObj(path);
  // we first check the name because if the name is in the cache
  // we just get out
  std::string name = j_obj["name"].get<std::string>();

  // quering the basic data
  m_isLoopable = j_obj["looping"].get<bool>();
  int start = j_obj["start"].get<int>();
  int end = j_obj["end"].get<int>();
  m_frameRate = j_obj["frame_rate"].get<float>();

  int posesSize = int(j_obj["poses"].size());
  m_poses.resize(posesSize);
  m_frameCount = posesSize;

  int counter = 0;
  // this is the counter for the joint in the pose currently
  // processed
  int sub_c;
  DirectX::XMVECTOR q;
  DirectX::XMFLOAT3 vp;
  int jointSize;

  DirectX::XMFLOAT3 zero_v(0, 0, 0);
  DirectX::XMFLOAT4 fzero(0, 0, 0, 0);
  DirectX::XMVECTOR zero_q = DirectX::XMLoadFloat4(&fzero);

  for (auto &pose : j_obj["poses"]) {
    jointSize = int(pose.size());

    AnimPose &currP = m_poses[counter];
    currP.m_local_pose.resize(jointSize);

    std::vector<JointPose> &joints = currP.m_local_pose;

    sub_c = 0;
    for (auto &joint : pose) {
      // extracting the pose
      // extracting the joint position
      vp = getValueIfInJson<DirectX::XMFLOAT3>(joint, "pos", zero_v);

      // extracting joint quaternion
      // to note I export quaternion as x,y,z,w,  is initialized as
      // w,x,y,z
      q = getValueIfInJson<DirectX::XMVECTOR>(joint, "quat", zero_q);

      joints[sub_c].m_rot = q;
      joints[sub_c].m_trans = vp;
      // scale hardcoded, not used for the time being
      joints[sub_c].m_scale = 1.0f;
      ++sub_c;
    }

    // poses[counter].m_local_pose = joints;
    ++counter;
  }

  return true;
}

inline DirectX::XMFLOAT3 lerp3(const DirectX::XMFLOAT3& v1,
	const DirectX::XMFLOAT3& v2, float amount)
{
	return DirectX::XMFLOAT3
	{ 
		((1.0f - amount) * v1.x) + (amount * v2.x),
		((1.0f - amount) * v1.y) + (amount * v2.y),
		((1.0f - amount) * v1.z) + (amount * v2.z),
	};

}
void AnimState::updateGlobalByAnim(long long stamp_ns) {
  assert(clip != nullptr);
  assert(stamp_ns >= 0);
  int globalSize = m_pose->m_globalPose.size();
  int localSize = m_pose->m_localPose.size();
  assert((globalSize == localSize));

  // we convert to seconds, since we need to count how many frames
  // passed and that is expressed in seconds
  float speed_time_multiplier = NANO_TO_SECONDS * multiplier;
  float delta = (stamp_ns - globalStartStamp) * speed_time_multiplier;
  // dividing the time elapsed since we started playing animation
  // and divide by the framerate so we know how many frames we played so far
  float frames_elapsed_f = delta / (clip->m_frameRate);
  int frames_elapsed = static_cast<int>(floor(frames_elapsed_f));
  // converting the frames in loop
  int start_idx = frames_elapsed % clip->m_frameCount;

  // convert counter to idx
  // we find the two frames we need to interpoalte to
  int end_idx = start_idx + 1;
  int end_range = clip->m_frameCount - 1;
  // if the end frame is out of the range it means needs to loop around
  if (end_idx > end_range) {
    end_idx = 0;
  }

  // extracting the two poses
  const AnimPose &start_p = clip->m_poses[start_idx];
  const AnimPose &end_p = clip->m_poses[end_idx];
  // here we find how much in the frame we are, we do that
  // by subtracting the frames elapsed in flaot minues the floored
  // value basically leaving us only with the decimal part as
  // it was a modf
  float interp = (frames_elapsed_f - float(frames_elapsed));

  //#pragma omp parallel for
  for (unsigned int i = 0; i < m_pose->m_skeleton->m_jointCount; ++i) {
    // interpolating all the bones in local space
    auto &j_s = start_p.m_local_pose[i];
    auto &j_e = end_p.m_local_pose[i];

    // we slerp the rotation and linearlly interpolate the
    // translation
	DirectX::XMVECTOR rot = DirectX::XMQuaternionSlerp(j_s.m_rot, j_e.m_rot, interp);

	DirectX::XMFLOAT3 pos = lerp3(j_s.m_trans, j_e.m_trans, interp);
    // the compiler should be able to omptimise out this
    // copy
    m_pose->m_localPose[i].m_rot = rot;
    m_pose->m_localPose[i].m_trans = pos;
  }
  // now that the anim has been blended I will compute the
  // matrices in worldspace (skin ready)
  m_pose->updateGlobalFromLocal();
}

} // namespace animation
