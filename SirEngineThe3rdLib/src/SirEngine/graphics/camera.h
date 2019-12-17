#pragma once
#include "SirEngine/engineConfig.h"
#include "SirEngine/globals.h"

#include "SirEngine/matrix.h"

namespace SirEngine {

struct CameraManipulationConfig {
  float m_panMultX;
  float m_panMultY;
  float m_rotateMultX;
  float m_rotateMultY;
  float m_zoomMult;
};

class Camera3DPivot final {

public:
  Camera3DPivot() = default;
  ~Camera3DPivot() = default;

  void setManipulationMultipliers(const CameraManipulationConfig& config){m_config = config;};
  void panCamera(float deltaX, float deltaY);
  void rotCamera(float deltaX, float deltaY);
  void zoomCamera(float deltaX);
  glm::mat4 getMVP(glm::mat4 modelM) const;
  glm::mat4 getMVPInverse(glm::mat4 modelM) const;
  glm::mat4 getViewInverse(glm::mat4 modelM) const;

  void spinCameraWorldYAxis(float angleInDegrees);

  void updateCamera() {
    // m_viewMatrix = DirectX::XMMatrixLookAtLH(posV, lookAtPosV, upVector);
  }

  [[nodiscard]] glm::vec3 getPosition() const { return glm::vec3(posV); }
  [[nodiscard]] glm::vec3 getLookAt() const { return glm::vec3(lookAtPosV); }
  [[nodiscard]] glm::vec4 getProjParams() const;

  inline void setPosition(const float x, const float y, const float z) {
    posV = glm::vec4(x, y, z, 1.0f);
  };
  inline void setLookAt(const float x, const float y, const float z) {
    lookAtPosV = glm::vec4(x, y, z, 1.0f);
  }

private:
  // Constants
  static constexpr glm::vec3 UP_VECTOR{0.0f, 1.0f, 0.0f};
  CameraManipulationConfig m_config{1.0f,1.0f,1.0f,1.0f,1.0f};
  //static constexpr float MOUSE_ROT_SPEED = 0.012f;
  //static constexpr float MOUSE_PAN_SPEED = 0.07f;

  glm::vec4 posV;
  glm::vec4 lookAtPosV;
};
} // namespace SirEngine
