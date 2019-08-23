#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>
namespace SirEngine {
struct Joint {
  DirectX::XMMATRIX m_inv_bind_pose;
  std::string m_name;
  unsigned char m_parent_id;
};

struct JointPose {
  DirectX::XMVECTOR m_rot;
  DirectX::XMFLOAT3 m_trans;
  float m_scale; // uniform scale only for now not included
};

struct Skeleton {
  unsigned int m_joint_count;
  std::vector<Joint> m_joints;
  std::string m_name;

  bool initialize(const std::string &path);
};

struct SkeletonPose {
  // pointer to the skeleton
  const Skeleton *m_skeleton;
  std::vector<JointPose> m_local_pose;
  // matrices ready to be uploaded to the shader
  std::vector<DirectX::XMMATRIX> m_global_pose;
  std::vector<DirectX::XMMATRIX> m_worldMat;

  /*
   * This functions will generates a global pose for all the bones,
   * the global pose is not the world position of the bones, it is the world
   * position multiplied by the inverse bind, basically the matrix that will
   * transform the vertex by the bone, so matrix ready for the skin cluster to
   * process the result will be stored in m_global_pose
   * @param alloc: the allocator to store temporary created matrices, since is a
   * stack allocator this is not thread safe due to the nature of the stack
   * allocator, allocating and freeing from multiple threads will make a mess
   * @Note : not thread safe
   */
  void updateGlobalFromLocal();
};

} // namespace SirEngine
