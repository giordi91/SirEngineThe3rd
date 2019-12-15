#include "SirEngine/matrix.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/graphicsDefines.h"

#include "glm/glm.hpp"
#include <glm/gtx/transform.hpp>

#include <DirectXMath.h>

namespace SirEngine {
// utility function to convert from dx -> glm and viceversa
glm::mat4 toGLM(const DirectX::XMMATRIX &mat) {
  glm::mat4 toReturn;
  assert(sizeof(mat) == sizeof(toReturn));
  memcpy(&toReturn, &mat, sizeof(mat));
  return toReturn;
}

glm::vec4 toGLM(const DirectX::XMVECTOR &vec) {
  return glm::vec4(vec.m128_f32[0], vec.m128_f32[1], vec.m128_f32[2],
                   vec.m128_f32[3]);
}

DirectX::XMMATRIX toDirectX(const glm::mat4 &mat) {
  auto matGlm = glm::transpose(mat);
  assert(sizeof(mat) == sizeof(matGlm));
  DirectX::XMMATRIX toReturn;
  memcpy(&toReturn, &mat, sizeof(mat));
  return toReturn;
}

glm::mat4 getPerspectiveMatrix(const int screenWidth, const int screenHeight) {

  // compute parameters
  // TODO fix hardcoded camera parameters
  constexpr float fieldOfView = SE_PI / 4.0f;
  float screenAspect =
      static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
  const float farP = 1000.0f;
  const float nearP = 0.001f;

  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
    auto xmat = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect,
                                                  farP, nearP);
    return toGLM(xmat);
  }

  auto m = glm::translate(glm::mat4(1.0f), glm::vec3(1, 2, 2));
  auto temp44 = glm::perspective(fieldOfView, screenAspect, farP, nearP);
  return temp44;
}

glm::mat4 getOrthoMatrix(const glm::vec3 minP, const glm::vec3 maxP) {
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
    auto xmat = DirectX::XMMatrixOrthographicLH(
        maxP.x - minP.x, maxP.y - minP.y, maxP.z, minP.z);
    return toGLM(xmat);
  }

  // NOTE: not tested!!!!
  return glm::ortho(minP.x, maxP.x, minP.y, maxP.y, maxP.z, minP.z);
}

glm::mat4 getLookAtMatrix(const glm::vec4 pos, const glm::vec4 lookAt,
                          const glm::vec3 upVec) {
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {

    DirectX::XMVECTOR posV = DirectX::XMVectorSet(pos.x, pos.y, pos.z, pos.w);
    DirectX::XMVECTOR lookAtPosV =
        DirectX::XMVectorSet(lookAt.x, lookAt.y, lookAt.z, lookAt.w);
    DirectX::XMVECTOR upVector =
        DirectX::XMVectorSet(upVec.x, upVec.y, upVec.z, 0.0f);
    auto toReturn = DirectX::XMMatrixLookAtLH(posV, lookAtPosV, upVector);
    return toGLM(toReturn);
  }

  return glm::lookAt(glm::vec3(pos), glm::vec3(lookAt), glm::vec3(upVec));
}

} // namespace SirEngine
