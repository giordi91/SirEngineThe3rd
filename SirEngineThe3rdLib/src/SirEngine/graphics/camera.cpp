#include "SirEngine/graphics/camera.h"

namespace SirEngine {

inline DirectX::XMVECTOR splatFloat(float value) {
  auto temp = DirectX::XMFLOAT4(value, value, value, value);
  return XMLoadFloat4(&temp);
}

const DirectX::XMVECTOR Camera3DPivot::upVector =
    DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0);
const float Camera3DPivot::MOUSE_ROT_SPEED_SCALAR = 0.012f;
const float Camera3DPivot::MOUSE_PAN_SPEED_SCALAR = 0.007f;
const DirectX::XMVECTOR Camera3DPivot::MOUSE_ROT_SPEED_VECTOR =
    DirectX::XMVectorSet(0.012f, 0.012f, 0.012f, 0.0f);
const DirectX::XMVECTOR Camera3DPivot::MOUSE_PAN_SPEED_VECTOR =
    DirectX::XMVectorSet(0.07f, 0.07f, 0.07f, 0.0f);

DirectX::XMMATRIX Camera3DPivot::getMVP(DirectX::XMMATRIX modelM) {
  const int screenW = globals::SCREEN_WIDTH;
  const int screenH = globals::SCREEN_HEIGHT;
  return XMMatrixMultiply(DirectX::XMMatrixMultiply(modelM, m_viewMatrix),
                          getProjCamera(screenW, screenH));
}

DirectX::XMMATRIX Camera3DPivot::getMVPInverse(DirectX::XMMATRIX modelM) {

  int screenW = globals::SCREEN_WIDTH;
  int screenH = globals::SCREEN_HEIGHT;
  auto mat = XMMatrixMultiply(DirectX::XMMatrixMultiply(modelM, m_viewMatrix),
                              getProjCamera(screenW, screenH));
  auto det = DirectX::XMMatrixDeterminant(mat);
  return XMMatrixInverse(&det, mat);
}
DirectX::XMMATRIX Camera3DPivot::getViewInverse(DirectX::XMMATRIX modelM) {
  auto mat = DirectX::XMMatrixMultiply(modelM, m_viewMatrix);
  auto det = DirectX::XMMatrixDeterminant(mat);
  return XMMatrixInverse(&det, mat);
}

void Camera3DPivot::spinCameraWorldYAxis(float angleInDegrees) {

  // this is a pivot camera, we want to rotate around the pivot
  auto rotationMatrix = DirectX::XMMatrixRotationY(angleInDegrees);

  // we translate the camera near the origin, compensating for look at position
  auto tempXPos = DirectX::XMVectorSubtract(posV, lookAtPosV);
  // next we rotate
  tempXPos = DirectX::XMVector3TransformCoord(tempXPos, rotationMatrix);
  // adding back the offset of the look at
  posV = DirectX::XMVectorAdd(tempXPos, lookAtPosV);
}

void Camera3DPivot::panCamera(float deltaX, float deltaY) {

  const auto updatedLookAtV = DirectX::XMVectorSubtract(lookAtPosV, posV);
  const auto cross = DirectX::XMVector3Cross(upVector, updatedLookAtV);
  const auto crossNorm = DirectX::XMVector3Normalize(cross);

  const auto newUp = DirectX::XMVector3Cross(crossNorm, updatedLookAtV);
  const auto newUpNorm = DirectX::XMVector3Normalize(newUp);

  const auto finalScaleX =
      DirectX::XMVectorMultiply(splatFloat(deltaX), MOUSE_PAN_SPEED_VECTOR);
  posV = DirectX::XMVectorMultiplyAdd(crossNorm, finalScaleX, posV);
  lookAtPosV = DirectX::XMVectorMultiplyAdd(crossNorm, finalScaleX, lookAtPosV);

  const auto finalScaleY =
      DirectX::XMVectorMultiply(splatFloat(deltaY), MOUSE_PAN_SPEED_VECTOR);
  posV = DirectX::XMVectorMultiplyAdd(newUpNorm, finalScaleY, posV);
  lookAtPosV = DirectX::XMVectorMultiplyAdd(newUpNorm, finalScaleY, lookAtPosV);
}

void Camera3DPivot::rotCamera(float deltaX, float deltaY) {
  deltaX *= MOUSE_ROT_SPEED_SCALAR;
  deltaY *= MOUSE_ROT_SPEED_SCALAR;

  DirectX::XMFLOAT3 lookAtView;
  DirectX::XMStoreFloat3(&lookAtView, lookAtPosV);
  const auto offsetForXRot =
      DirectX::XMVectorSet(0.0f, lookAtView.y, 0.0f, 0.0f);

  const auto rotXMatrix = DirectX::XMMatrixRotationAxis(upVector, -deltaX);
  auto tempXPos = DirectX::XMVector3TransformCoord(posV, rotXMatrix);

  // getting cross
  const auto cross = DirectX::XMVector3Cross(upVector, tempXPos);
  const auto crossNorm = DirectX::XMVector3Normalize(cross);

  const auto rotYMatrix = DirectX::XMMatrixRotationAxis(crossNorm, deltaY);
  tempXPos = DirectX::XMVectorSubtract(tempXPos, lookAtPosV);
  posV = DirectX::XMVector3TransformCoord(tempXPos, rotYMatrix);
  posV = DirectX::XMVectorAdd(posV, lookAtPosV);
}

void Camera3DPivot::zoomCamera(float deltaX) {
  const auto updatedLookAtV = DirectX::XMVectorSubtract(lookAtPosV, posV);
  const auto updatedLookAtVNorm = DirectX::XMVector3Normalize(updatedLookAtV);
  const auto finalScaleX =
      DirectX::XMVectorMultiply(splatFloat(-deltaX), MOUSE_PAN_SPEED_VECTOR);
  posV = DirectX::XMVectorMultiplyAdd(updatedLookAtVNorm, finalScaleX, posV);
}

} // namespace SirEngine
