#pragma once
#include "SirEngine/globals.h"
#include <directxmath.h>

namespace SirEngine {

class Camera3DPivot final {

public:
  Camera3DPivot() = default;
  ~Camera3DPivot() = default;

  void panCamera(float deltaX, float deltaY);
  void rotCamera(float deltaX, float deltaY);
  void zoomCamera(float deltaX);
  DirectX::XMMATRIX getMVP(DirectX::XMMATRIX modelM);
  DirectX::XMMATRIX getMVPInverse(DirectX::XMMATRIX modelM);
  DirectX::XMMATRIX getViewInverse(DirectX::XMMATRIX modelM);

  inline DirectX::XMMATRIX getViewMatrix() const { return m_viewMatrix; };

  void updateCamera() {
    m_viewMatrix = DirectX::XMMatrixLookAtLH(posV, lookAtPosV, upVector);
  }

  inline DirectX::XMMATRIX getProjCamera(int screenWidth,
                                         int screenHeight) const {
    constexpr float fieldOfView = DirectX::XM_PI / 4.0f;
    float screenAspect =
        static_cast<float>(screenWidth) / static_cast<float>(screenHeight);

    return DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, 0.001f,
                                             100.0f);
  };

  inline DirectX::XMFLOAT3 getPosition() const {
    DirectX::XMFLOAT3 toReturn;
    DirectX::XMStoreFloat3(&toReturn, posV);
    return toReturn;
  }
  inline DirectX::XMFLOAT3 getLookAt() const {
    DirectX::XMFLOAT3 toReturn;
    DirectX::XMStoreFloat3(&toReturn, lookAtPosV);
    return toReturn;
  }
  inline DirectX::XMFLOAT4 getProjParams() const {
    // preparing camera values for deferred
    int screenW = globals::SCREEN_WIDTH;
    int screenH = globals::SCREEN_HEIGHT;
    auto proj = getProjCamera(screenW, screenH);
    DirectX::XMFLOAT4X4 projView;
    DirectX::XMStoreFloat4x4(&projView, proj);
    DirectX::XMFLOAT4 perspValues;
    perspValues.x = 1.0f / projView.m[0][0];
    perspValues.y = 1.0f / projView.m[1][1];
    perspValues.z = projView.m[3][2];
    perspValues.w = -projView.m[2][2];
    return perspValues;
  }
  inline void setPosition(float x, float y, float z) {
    posV = DirectX::XMVectorSet(x, y, z, 1.0f);
  };
  inline void setLookAt(float x, float y, float z) {
    lookAtPosV = DirectX::XMVectorSet(x, y, z, 1.0f);
  }

private:
  // Constants
  static const DirectX::XMFLOAT3 upVec3;
  static const DirectX::XMVECTOR upVector;
  static const float MOUSE_ROT_SPEED_SCALAR;
  static const float MOUSE_PAN_SPEED_SCALAR;
  static const DirectX::XMVECTOR MOUSE_ROT_SPEED_VECTOR;
  static const DirectX::XMVECTOR MOUSE_PAN_SPEED_VECTOR;
  DirectX::XMVECTOR posV;
  DirectX::XMVECTOR lookAtPosV;
  DirectX::XMMATRIX m_viewMatrix;
};
} // namespace SirEngine
