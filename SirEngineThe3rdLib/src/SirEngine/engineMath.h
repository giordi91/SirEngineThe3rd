#pragma once
#include <intrin.h>

#include "glm/glm.hpp"

namespace SirEngine {

inline uint32_t getFirstBitSet(const uint32_t value) {
#if defined(_MSC_VER)
  unsigned long index;
  _BitScanForward(&index, value);
  return index;
#elif defined(__clang__) && !defined(CC_CLANG)
  assert(0);
#elif (defined(__GNUC__) || defined(__GNUG__)) && !defined(CC_GCC)
  assert(0);

#endif
}
// the main supported library is GLM everything should be done in term of GLM
// no matter the API
// GLM is column major
glm::mat4 getPerspectiveMatrix(const int screenWidth, const int screenHeight,
                               const float vfov, const float nearP,
                               const float farP);
glm::mat4 getOrthoMatrix(const glm::vec3 minP, const glm::vec3 maxP);
glm::mat4 getLookAtMatrix(glm::vec4 pos, glm::vec4 lookAt, glm::vec3 upVec);

}  // namespace SirEngine