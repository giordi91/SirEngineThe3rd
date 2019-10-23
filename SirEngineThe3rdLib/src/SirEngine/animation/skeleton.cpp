#include "SirEngine/animation/skeleton.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/runtimeString.h"
namespace SirEngine {

static const char *SKELETON_KEY_DATA = "data";
static const char *SKELETON_KEY_MATRIX = "mat";
static const char *SKELETON_KEY_INDEX = "idx";
static const char *SKELETON_KEY_NAME = "name";

inline DirectX::XMVECTOR expandF3ToVec(const DirectX::XMFLOAT3 f3) {
  DirectX::XMFLOAT4 fvec(f3.x, f3.y, f3.z, 1.0f);
  return DirectX::XMLoadFloat4(&fvec);
}

bool Skeleton::loadFromFile(const char *path) {

  auto jObj = getJsonObj(path);

  // extracting how many bones
  const auto size = static_cast<uint32_t>(jObj[SKELETON_KEY_DATA].size());

  // if not loaded we start to read in the joints, first we allocated
  // the needed memory
  m_jointsWolrdInv.resize(size);
  m_parentIds.resize(size);
  m_names.resize(size);
  DirectX::XMMATRIX *joints = m_jointsWolrdInv.data();
  int *parentIds = m_parentIds.data();
  const char **names = m_names.data();
  m_jointCount = size;

  // then we start looping
  int counter = 0;
  const DirectX::XMMATRIX defaultMatrix = DirectX::XMMatrixIdentity();
  for (const auto &e : jObj[SKELETON_KEY_DATA]) {

    // then we start extracting the data from the joint
    auto jnt = nlohmann::json(e);
    joints[counter] = getValueIfInJson<DirectX::XMMATRIX>(
        jnt, SKELETON_KEY_MATRIX, defaultMatrix);

    // bone parent id
    const int idx = jnt[SKELETON_KEY_INDEX].get<int>();
    parentIds[counter] = idx;

    // bone name
    const auto currentName = jnt[SKELETON_KEY_NAME].get<std::string>();
    names[counter] = persistentString(currentName.c_str());
    ++counter;
  }

  // add skeleton to the map
  m_name = persistentString(jObj[SKELETON_KEY_NAME].get<std::string>().c_str());

  return true;
}

void SkeletonPose::updateGlobalFromLocal() {

  // establishing the invariant, where every bone
  // in the list appears after the parent,so meanwhile we fill
  // the global pose list we can access to get the world matrices
  // since we know whatever parent is already processed

  // here we compute the root
  // those conversions might actually hurt performance but
  // for now is not a bottle neck since I evaluate few skeletons
  // only
  // TODO maybe remove glm or do the computation manually so to avoid
  // intermediate copies and conversions between data-types etc
  DirectX::XMVECTOR &r = m_localPose[0].m_rot;
  DirectX::XMMATRIX qM = DirectX::XMMatrixRotationQuaternion(r);
  // auto qM = glm::toMat4(r);
  qM.r[3] = expandF3ToVec(m_localPose[0].m_trans);

  m_worldMat[0] = qM;
  m_globalPose[0] = qM * m_skeleton->m_jointsWolrdInv.getConstRef(0);

  for (uint32_t i = 1; i < m_skeleton->m_jointCount; ++i) {
    const int idx = m_skeleton->m_parentIds[i];

    // here just getting a reference to avoid to nest too much
    // the compiler should optimize that away
    DirectX::XMVECTOR &rr = m_localPose[i].m_rot;
    qM = DirectX::XMMatrixRotationQuaternion(rr);
    // setting the translation
    qM.r[3] = expandF3ToVec(m_localPose[i].m_trans);
    // here I compute the global pose so this is the world matrix
    m_worldMat[i] = DirectX::XMMatrixMultiply(qM, m_worldMat[idx]);
    // in the global pose I will store the matrix ready for vertex
    // transformation
    m_globalPose[i] = DirectX::XMMatrixTranspose(DirectX::XMMatrixMultiply(
        m_skeleton->m_jointsWolrdInv[i], m_worldMat[i]));
  }
}
} // namespace SirEngine
