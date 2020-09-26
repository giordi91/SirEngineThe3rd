// NOTE needed in order to get range between 0-1 and have inverted depth working
// properly in vulkan
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "SirEngine/engineMath.h"

#include <DirectXMath.h>

#include <glm/gtx/transform.hpp>

#include "SirEngine/engineConfig.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "glm/glm.hpp"

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
  DirectX::XMMATRIX toReturn{};
  memcpy(&toReturn, &mat, sizeof(mat));
  return toReturn;
}

glm::mat4 getPerspectiveMatrix(const int screenWidth, const int screenHeight,
                               const float vfov, const float nearP, const float farP) {
  // compute parameters
  float screenAspect =
      static_cast<float>(screenWidth) / static_cast<float>(screenHeight);

  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
    auto xmat =
        DirectX::XMMatrixPerspectiveFovLH(vfov, screenAspect, nearP, farP);
    return toGLM(xmat);
  }

  auto temp44 = glm::perspective(vfov, screenAspect, nearP, farP);
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

}  // namespace SirEngine
