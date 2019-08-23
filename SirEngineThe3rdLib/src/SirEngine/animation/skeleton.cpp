#include "SirEngine/animation/skeleton.h"
#include "SirEngine/fileUtils.h"
#include <string>

namespace SirEngine {
inline DirectX::XMVECTOR expandF3ToVec(const DirectX::XMFLOAT3 f3) {
  DirectX::XMFLOAT4 fvec(f3.x, f3.y, f3.z, 1.0f);
  return DirectX::XMLoadFloat4(&fvec);
}

bool Skeleton::initialize(const std::string &path) {

  auto j_obj = getJsonObj(path);

  // extracting how many bones
  size_t size = j_obj["data"].size();
  // sk name
  std::string name = j_obj["name"].get<std::string>();

  /*
  // lets check wheter the skeleton has been loaded already
  // or not, if it is we just return a pointer to it
  auto res = m_skeleton_map.find(name);
  if (res != m_skeleton_map.end()) {
    return m_skeleton_map[name];
  }
  */

  // if not loaed we start to read in the joints, first we allocated
  // the needed memory
  m_joints.resize(size);
  m_joint_count = size;
  // anim::Joint *joints =
  //    static_cast<Joint *>(m_joints_alloc.allocate(sizeof(Joint) * size));

  // then we start looping
  int counter = 0;
  DirectX::XMMATRIX defaultMatrix = DirectX::XMMatrixIdentity();
  for (const auto &e : j_obj["data"]) {

    // lets grab the ref to the memory we allocated
    Joint &jntStr = m_joints[counter];

    // then we start extracting the data from the joint
    auto jnt = nlohmann::json(e);
    jntStr.m_inv_bind_pose =
        getValueIfInJson<DirectX::XMMATRIX>(jnt, "mat", defaultMatrix);

    // bone parent id
    const int idx = jnt["idx"].get<int>();
    jntStr.m_parent_id = idx;

    // bone name
    jntStr.m_name = jnt["name"].get<std::string>();
    ++counter;
  }

  // add skeleton to the map
  m_name = j_obj["name"].get<std::string>();

  return true;
}

void SkeletonPose::updateGlobalFromLocal() {
  // allocating enough memory for the temporary world matrices
  // std::vector<DirectX::XMMATRIX> m_worldMat;
  m_worldMat.resize(m_skeleton->m_joint_count);
  // estabilishing the invariant, where every bone
  // in the list appears after the parent,so meanwhile we fill
  // the globalpose list we can access to get the world matrices
  // since we know whatever parent is already processed

  // here we compute the root
  // those convertions might actually hurt performance but
  // for now is not a bottle neck since i evalaute few skeletons
  // only
  // TODO maybe remove glm or do the computation manually so to avoid
  // intermediate copies and convertions between datatypes etc
  DirectX::XMVECTOR &r = m_local_pose[0].m_rot;
  DirectX::XMMATRIX qM = DirectX::XMMatrixRotationQuaternion(r);
  // auto qM = glm::toMat4(r);
  qM.r[3] = expandF3ToVec(m_local_pose[0].m_trans);

  m_worldMat[0] = qM;
  m_global_pose[0] = qM * m_skeleton->m_joints[0].m_inv_bind_pose;

  for (unsigned int i = 1; i < m_skeleton->m_joint_count; ++i) {
    const int idx = m_skeleton->m_joints[i].m_parent_id;

    // here just getting a reference to avoid to nest too much
    // the compiler should optimize that away
    DirectX::XMVECTOR &rr = m_local_pose[i].m_rot;
    qM = DirectX::XMMatrixRotationQuaternion(rr);
    // qM = glm::toMat4(rr);

    auto v4 = expandF3ToVec(m_local_pose[i].m_trans);
    qM.r[3] = v4;
    // qM[3] = glm::vec4(m_local_pose[i].m_trans, 1.0f);
    // here I compute the global pose so this is the world matrix
    // m_worldMat[i] = m_worldMat[idx] * qM;
    m_worldMat[i] = DirectX::XMMatrixMultiply(qM, m_worldMat[idx]);
    // in the global pose I will store the matrix ready for vertex
    // transformation m_global_pose[i] = m_worldMat[i] *
    // m_skeleton->m_joints[i].m_inv_bind_pose;
    m_global_pose[i] = DirectX::XMMatrixTranspose(DirectX::XMMatrixMultiply(
        m_skeleton->m_joints[i].m_inv_bind_pose, m_worldMat[i]));
  }
}
} // namespace SirEngine