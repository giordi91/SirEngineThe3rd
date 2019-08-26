#pragma once
#include <DirectXMath.h>
#include <vector>
#include "SirEngine/memory/resizableVector.h"
namespace SirEngine {

struct JointPose {
  DirectX::XMVECTOR m_rot;
  DirectX::XMFLOAT3 m_trans;
  float m_scale; // uniform scale only for now not included
};

struct Skeleton {
  //Skeleton(ThreeSizesPool *allocator = nullptr)
  //    : m_jointCount(0), m_jointsWolrdInv(20, allocator), m_names(20,allocator),m_parentIds(20,allocator){};
  unsigned int m_jointCount;
  ResizableVector<DirectX::XMMATRIX> m_jointsWolrdInv;
  ResizableVector<const char*> m_names;
  ResizableVector<int> m_parentIds;
  const char* m_name;

  bool loadFromFile(const char* path);
};

struct SkeletonPose {
  // pointer to the skeleton
  const Skeleton *m_skeleton;
  std::vector<JointPose> m_localPose;
  // matrices ready to be uploaded to the shader
  std::vector<DirectX::XMMATRIX> m_globalPose;
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
