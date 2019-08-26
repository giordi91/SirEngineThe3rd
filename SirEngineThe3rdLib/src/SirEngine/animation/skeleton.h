#pragma once
#include "SirEngine/globals.h"
#include "SirEngine/memory/resizableVector.h"
#include <DirectXMath.h>
namespace SirEngine {

struct JointPose {
  DirectX::XMVECTOR m_rot;
  DirectX::XMFLOAT3 m_trans;
  float m_scale; // uniform scale only for now not included
};

struct Skeleton {
  static constexpr uint32_t PREALLOCATION_SIZE = 32;
  explicit Skeleton(
      PersistantAllocatorType *allocator = globals::PERSISTANT_ALLOCATOR)
      : m_jointCount(0), m_jointsWolrdInv(PREALLOCATION_SIZE, allocator),
        m_names(PREALLOCATION_SIZE, allocator),
        m_parentIds(PREALLOCATION_SIZE, allocator), m_name(nullptr){};
  unsigned int m_jointCount;
  ResizableVector<DirectX::XMMATRIX> m_jointsWolrdInv;
  ResizableVector<const char *> m_names;
  ResizableVector<int> m_parentIds;
  const char *m_name;

  bool loadFromFile(const char *path);
};

struct SkeletonPose {
  static constexpr uint32_t PREALLOCATION_SIZE = 32;

  explicit SkeletonPose(
      PersistantAllocatorType *allocator = globals::PERSISTANT_ALLOCATOR)
      : m_skeleton(nullptr), m_localPose(PREALLOCATION_SIZE, allocator),
        m_globalPose(PREALLOCATION_SIZE, allocator),
        m_worldMat(PREALLOCATION_SIZE, allocator) {}
  // pointer to the skeleton
  const Skeleton *m_skeleton;
  ResizableVector<JointPose> m_localPose;
  // matrices ready to be uploaded to the shader
  ResizableVector<DirectX::XMMATRIX> m_globalPose;
  ResizableVector<DirectX::XMMATRIX> m_worldMat;

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
