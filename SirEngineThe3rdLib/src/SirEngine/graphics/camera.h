#pragma once
#include "SirEngine/globals.h"
#include <directxmath.h>

namespace SirEngine {

// virtual void setCameraMatrixToShader(DirectX::XMMATRIX modelMatrix);
// virtual void setupCameraBuffer(ID3D11DeviceContext *context, int slot);

class Camera3dPivot {

public:
  Camera3dPivot() = default;
  virtual ~Camera3dPivot() = default;

  void panCamera(float deltaX, float deltaY);
  void rotCamera(float deltaX, float deltaY);
  void zoomCamera(float deltaX);
  DirectX::XMMATRIX getMVP(DirectX::XMMATRIX modelM);
  DirectX::XMMATRIX getMVPInverse(DirectX::XMMATRIX modelM);
  DirectX::XMMATRIX getViewInverse(DirectX::XMMATRIX modelM);

  inline DirectX::XMMATRIX getViewMatrix() const { return m_viewMatrix; };

  void Render() {
    m_viewMatrix = DirectX::XMMatrixLookAtLH(posV, lookAtPosV, upVector);
  }
  DirectX::XMMATRIX getProjCamera(int screenWidth, int screenHeight) {
    constexpr float fieldOfView = DirectX::XM_PI / 4.0f;
    float screenAspect = (float)screenWidth / (float)screenHeight;

    return DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, 0.001f,
                                             100.0f);
    // m_constants->SCREEN_NEAR,
    // m_constants->SCREEN_DEPTH);
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
  inline DirectX::XMFLOAT4 getProjParams() {
    // preparing camera values for deferred
    int screenW = Globals::SCREEN_WIDTH;
    int screenH = Globals::SCREEN_HEIGHT;
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
    // posV = DirectX::XMVectorSet(x, y, z, 1.0f);
    posV = DirectX::XMLoadFloat3(&DirectX::XMFLOAT3(x, y, z));
  };
  inline void setLookAt(float x, float y, float z) {
    lookAtPosV = DirectX::XMLoadFloat3(&DirectX::XMFLOAT3(x, y, z));
  }

  // inline ID3D11Buffer **getPointerToCameraBuffer() {
  //  return &m_perspValuesBuffer;
  //}

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
  // ID3D11Buffer *m_cameraBuffer;
  //// buffer used to setup and prepare the camera
  // PerspectiveBufferDef m_persp;
  // ID3D11Buffer *m_perspValuesBuffer = nullptr;
}; // namespace SirEngine
} // namespace SirEngine
