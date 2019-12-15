#include "SirEngine/animation/skeleton.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/runtimeString.h"
#undef max
#include <glm/gtx/quaternion.hpp>

namespace SirEngine {

static const char *SKELETON_KEY_DATA = "data";
static const char *SKELETON_KEY_MATRIX = "mat";
static const char *SKELETON_KEY_INDEX = "idx";
static const char *SKELETON_KEY_NAME = "name";

bool Skeleton::loadFromFile(const char *path) {

  auto jObj = getJsonObj(path);

  // extracting how many bones
  const auto size = static_cast<uint32_t>(jObj[SKELETON_KEY_DATA].size());

  // if not loaded we start to read in the joints, first we allocated
  // the needed memory
  m_jointsWolrdInv.resize(size);
  m_parentIds.resize(size);
  m_names.resize(size);
  glm::mat4 *joints = m_jointsWolrdInv.data();
  int *parentIds = m_parentIds.data();
  const char **names = m_names.data();
  m_jointCount = size;

  // then we start looping
  int counter = 0;
  const glm::mat4 defaultMatrix(1.0f);
  for (const auto &e : jObj[SKELETON_KEY_DATA]) {

    // then we start extracting the data from the joint
    auto jnt = nlohmann::json(e);
    joints[counter] = getValueIfInJson<glm::mat4>(
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

void SkeletonPose::updateGlobalFromLocal(
    const glm::mat4 transform) const {

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
  const glm::quat &r = m_localPose[0].m_rot;
  glm::mat4 qM = glm::toMat4(r);
  // patching the rotation in
  qM[3].x = m_localPose[0].m_trans.x;
  qM[3].y = m_localPose[0].m_trans.y;
  qM[3].z = m_localPose[0].m_trans.z;

  // we want to hit the root by our transform, which will move the
  // skeleton around the world
  qM = transform* qM;

  m_worldMat[0] = qM;
  m_globalPose[0] = glm::transpose(qM * m_skeleton->m_jointsWolrdInv.getConstRef(0)); 
      
  for (uint32_t i = 1; i < m_skeleton->m_jointCount; ++i) {
    const int idx = m_skeleton->m_parentIds[i];

    // here just getting a reference to avoid to nest too much
    // the compiler should optimize that away
    const glm::quat &rr = m_localPose[i].m_rot;
    glm::mat4 qM2 = glm::toMat4(rr);
    // setting the translation
    qM2[3].x = m_localPose[i].m_trans.x;
    qM2[3].y = m_localPose[i].m_trans.y;
    qM2[3].z = m_localPose[i].m_trans.z;
    // here I compute the global pose so this is the world matrix
    m_worldMat[i] =m_worldMat[idx]* qM2;
    // in the global pose I will store the matrix ready for vertex
    // transformation
    m_globalPose[i] = glm::transpose(m_worldMat[i]*m_skeleton->m_jointsWolrdInv[i]);
  }
}
} // namespace SirEngine
