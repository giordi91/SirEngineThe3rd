#pragma once

#include "SirEngine/engineConfig.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/input.h"
#include "SirEngine/matrix.h"
#include "graphicsDefines.h"

namespace SirEngine {

struct CameraManipulationConfig {
  float m_panMultX;
  float m_panMultY;
  float m_rotateMultX;
  float m_rotateMultY;
  float m_zoomMult;
};

class CameraController {
 public:
  CameraController() = default;
  virtual ~CameraController() = default;

  void setManipulationMultipliers(const CameraManipulationConfig& config) {
    m_config = config;
  };
  [[nodiscard]] const CameraBuffer& getCameraBuffer() const {
    return m_cameraBuffer;
  }
  void setCameraPhyisicalParameters(const float vfovDegrees, const float nearP,
                                    const float farP) {
    m_vfov = vfovDegrees * TO_RAD;
    m_near = nearP;
    m_far = farP;
  }

  virtual void updateCamera() = 0;

 protected:
  float m_vfov = SE_PI / 4.0f;
  float m_near = 0.001f;
  float m_far = 100.0f;
  CameraBuffer m_cameraBuffer{};
  CameraManipulationConfig m_config{1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
};

class Camera3DPivot final : public CameraController {
 public:
  Camera3DPivot() : CameraController(){};
  virtual ~Camera3DPivot() = default;

  void spinCameraWorldYAxis(float angleInDegrees);

  void manipulateCamera() {
    const float deltaX = previousX - globals::INPUT->m_mousePosX;
    const float deltaY = previousY - globals::INPUT->m_mousePosY;
    bool leftDown = globals::INPUT->m_mouse[MOUSE_BUTTONS::LEFT];
    bool middleDown = globals::INPUT->m_mouse[MOUSE_BUTTONS::MIDDLE];
    bool rightDown = globals::INPUT->m_mouse[MOUSE_BUTTONS::RIGHT];
    if (leftDown) {
      globals::ACTIVE_CAMERA->rotCamera(deltaX, deltaY);
    } else if (middleDown) {
      globals::ACTIVE_CAMERA->panCamera(deltaX, deltaY);
    } else if (rightDown) {
      globals::ACTIVE_CAMERA->zoomCamera(deltaX);
    }

    // storing old position
    previousX = globals::INPUT->m_mousePosX;
    previousY = globals::INPUT->m_mousePosY;
  }

  void updateCameraBuffer() {
    m_cameraBuffer.vFov = m_vfov;

    auto pos = getPosition();
    m_cameraBuffer.position = glm::vec4(pos, 1.0f);

    // not super happy about this, but it is better to do it here, so
    // computation is consistent and we only transpose to the very end
    if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::VULKAN) {
      m_cameraBuffer.MVP = getMVP(glm::mat4(1.0));
      m_cameraBuffer.ViewMatrix = getViewInverse(glm::mat4(1.0));
      m_cameraBuffer.VPinverse = getMVPInverse(glm::mat4(1.0));
    } else {
      m_cameraBuffer.MVP = glm::transpose(getMVP(glm::mat4(1.0)));
      m_cameraBuffer.ViewMatrix =
          glm::transpose(getViewInverse(glm::mat4(1.0)));
      m_cameraBuffer.VPinverse = glm::transpose(getMVPInverse(glm::mat4(1.0)));
    }
    m_cameraBuffer.perspectiveValues = getProjParams();
  }

  void updateCamera() override {
    manipulateCamera();
    updateCameraBuffer();
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
  void panCamera(float deltaX, float deltaY);
  void rotCamera(float deltaX, float deltaY);
  void zoomCamera(float deltaX);
  glm::mat4 getMVP(glm::mat4 modelM) const;
  glm::mat4 getMVPInverse(glm::mat4 modelM) const;
  glm::mat4 getViewInverse(glm::mat4 modelM) const;

 private:
  // Constants
  static constexpr glm::vec3 UP_VECTOR{0.0f, 1.0f, 0.0f};

  glm::vec4 posV;
  glm::vec4 lookAtPosV;
  float previousX = 0;
  float previousY = 0;
};
}  // namespace SirEngine
