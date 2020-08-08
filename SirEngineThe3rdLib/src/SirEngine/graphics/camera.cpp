#include "SirEngine/graphics/camera.h"

#include <glm/gtx/transform.hpp>

#include "../../../../Tests/src/graphNodesDefinitions.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/matrix.h"
#include "glm/glm.hpp"

namespace SirEngine {

glm::mat4 Camera3DPivot::getMVP(const glm::mat4 modelM) const {
  const int screenW = globals::ENGINE_CONFIG->m_windowWidth;
  const int screenH = globals::ENGINE_CONFIG->m_windowHeight;

  // By default the engine uses inverted floating point depth
  const glm::mat4 projectionMatrix =
      getPerspectiveMatrix(screenW, screenH, m_vfov, m_far, m_near);
  const glm::mat4 viewMatrix = getLookAtMatrix(posV, lookAtPosV, UP_VECTOR);
  const glm::mat4 vp = projectionMatrix * (viewMatrix * modelM);
  return vp;
}

glm::mat4 Camera3DPivot::getMVPInverse(const glm::mat4 modelM) const {
  const int screenW = globals::ENGINE_CONFIG->m_windowWidth;
  const int screenH = globals::ENGINE_CONFIG->m_windowHeight;

  // By default the engine uses inverted floating point depth
  const glm::mat4 projectionMatrix =
      getPerspectiveMatrix(screenW, screenH, m_vfov, m_far, m_near);
  const glm::mat4 viewMatrix = getLookAtMatrix(posV, lookAtPosV, UP_VECTOR);
  const glm::mat4 vp = projectionMatrix * viewMatrix * modelM;
  return glm::inverse(vp);
}

glm::mat4 Camera3DPivot::getViewInverse(const glm::mat4 modelM) const {
  const auto viewMatrix = getLookAtMatrix(posV, lookAtPosV, UP_VECTOR);
  return glm::inverse(viewMatrix) * modelM;
}

void Camera3DPivot::spinCameraWorldYAxis(const float angleInDegrees) {
  // this is a pivot camera, we want to rotate around the pivot
  auto rotationMatrix =
      glm::rotate(glm::mat4(1.0), angleInDegrees * TO_RAD, UP_VECTOR);

  // we translate the camera near the origin, compensating for look at position
  auto compensatedPosition = posV - lookAtPosV;
  // next we rotate
  compensatedPosition = rotationMatrix * compensatedPosition;
  posV = compensatedPosition + lookAtPosV;
}

glm::vec4 Camera3DPivot::getProjParams() const {
  // preparing camera values for deferred
  int screenW = globals::ENGINE_CONFIG->m_windowWidth;
  int screenH = globals::ENGINE_CONFIG->m_windowHeight;
  // By default the engine uses inverted floating point depth
  const auto projectionMatrix =
      getPerspectiveMatrix(screenW, screenH, m_vfov, m_far, m_near);

  glm::vec4 perspValues;
  perspValues.x = 1.0f / projectionMatrix[0][0];
  perspValues.y = 1.0f / projectionMatrix[1][1];
  perspValues.z = projectionMatrix[3][2];
  perspValues.w = -projectionMatrix[2][2];
  return perspValues;
}

void Camera3DPivot::panCamera(float deltaX, float deltaY) {
  const auto updatedLookAtV = glm::vec3(lookAtPosV - posV);
  const glm::vec3 crossV = glm::cross(UP_VECTOR, updatedLookAtV);
  const glm::vec3 crossNorm = glm::normalize(crossV);

  const glm::vec3 newUp = glm::cross(crossNorm, updatedLookAtV);
  const glm::vec3 newUpNorm = glm::normalize(newUp);

  const glm::vec3 finalScaleX = glm::vec3(deltaX) * m_config.m_panMultX;
  const auto offestX = glm::vec4(crossNorm * finalScaleX, 0.0f);

  posV += offestX;
  lookAtPosV += offestX;

  const auto finalScaleY = glm::vec3(deltaY) * m_config.m_panMultY;
  const auto offestY = glm::vec4(newUpNorm * finalScaleY, 0.0f);
  posV += offestY;
  lookAtPosV += offestY;
}

void Camera3DPivot::rotCamera(const float deltaX, const float deltaY) {
  // compute a rotation matrix on the Y axis and apply transformation
  glm::mat4 rotXMatrix =
      glm::rotate(glm::mat4(1.0), deltaX * m_config.m_rotateMultX, UP_VECTOR);
  auto rotatedXPos = rotXMatrix * (posV - lookAtPosV);

  // getting cross to compute the rotation up and down
  const auto crossNorm =
      glm::normalize(glm::cross(UP_VECTOR, glm::vec3(rotatedXPos)));

  // getting the rotation on the cross axis and apply the transformation
  glm::mat4 rotYMatrix =
      glm::rotate(glm::mat4(1.0), deltaY * m_config.m_rotateMultY, crossNorm);

  posV = (rotYMatrix * rotatedXPos) + lookAtPosV;
  posV.w = 1.0;
}

void Camera3DPivot::zoomCamera(const float deltaX) {
  const auto updatedLookAtV = glm::normalize(lookAtPosV - posV);
  const auto delta = updatedLookAtV * (deltaX * m_config.m_zoomMult);
  posV += delta;
}
}  // namespace SirEngine
