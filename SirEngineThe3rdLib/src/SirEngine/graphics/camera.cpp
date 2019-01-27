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

  int screenW = globals::SCREEN_WIDTH;
  int screenH = globals::SCREEN_HEIGHT;
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

// void Camera::setupCameraBuffer(ID3D11DeviceContext *context, int slot) {
//  // preparing camera values for deferred
//  auto *rendManager = rendering::RenderingManager::get_instance();
//  int screenW = rendManager->m_screenWidth;
//  int screenH = rendManager->m_screenHeight;
//  auto *camera = rendManager->m_mainCamera;
//  auto proj = getProjCamera(screenW, screenH);
//
//  auto viewM = camera->getViewInverse(DirectX::XMMatrixIdentity());
//  auto viewInv = DirectX::XMMatrixTranspose(viewM);
//
//  m_persp.perspectiveValues = camera->getProjParams();
//  m_persp.viewInv = viewInv;
//  auto p = camera->getPosition();
//  m_persp.cameraPosition = DirectX::XMFLOAT4(p.x, p.y, p.z, 1.0f);
//
//  // setup persp values
//  D3D11_MAPPED_SUBRESOURCE mappedResource;
//  HRESULT result = context->Map(m_perspValuesBuffer, 0,
//  D3D11_MAP_WRITE_DISCARD,
//                                0, &mappedResource);
//  if (FAILED(result)) {
//    assert(0);
//    return;
//  }
//  memcpy(mappedResource.pData, &m_persp, sizeof(PerspectiveBufferDef));
//  context->Unmap(m_perspValuesBuffer, 0);
//  context->PSSetConstantBuffers(slot, 1, &m_perspValuesBuffer);
//}

// void Camera::setCameraMatrixToShader(DirectX::XMMATRIX modelMatrix) {
//  HRESULT result;
//  D3D11_MAPPED_SUBRESOURCE mappedResource;
//  ObjectBufferDef *dataPtr;
//  unsigned int bufferNumber;
//
//  auto *d3d = D3DClass::get_instance();
//  auto *dev_context = d3d->GetDeviceContext();
//  auto *rendManager = RenderingManager::get_instance();
//  int screenW = rendManager->m_screenWidth;
//  int screenH = rendManager->m_screenHeight;
//  DirectX::XMMATRIX mvp = getMVP(modelMatrix);
//  mvp = XMMatrixTranspose(mvp);
//  // Lock the constant buffer so it can be written to.
//  result = dev_context->Map(m_cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0,
//                            &mappedResource);
//  if (FAILED(result)) {
//    assert(0);
//    return;
//  }
//
//  // Get a pointer to the data in the constant buffer.
//  dataPtr = (ObjectBufferDef*)mappedResource.pData;
//  // Copy the matrices into the constant buffer.
//  dataPtr->MVP = mvp;
//  dataPtr->normalMatrix = XMMatrixTranspose(m_viewMatrix);
//  // Unlock the constant buffer.
//  dev_context->Unmap(m_cameraBuffer, 0);
//
//  // Set the position of the constant buffer in the vertex shader.
//  bufferNumber = 0;
//
//  // Finanly set the constant buffer in the vertex shader with the updated
//  // values.
//  dev_context->VSSetConstantBuffers(bufferNumber, 1, &m_cameraBuffer);
//}

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
