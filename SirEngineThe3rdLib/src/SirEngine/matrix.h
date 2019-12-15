#pragma once
#include "glm/glm.hpp"

namespace SirEngine {

// the main supported library is GLM everything should be done in term of GLM
// no matter the API
// GLM is column major
glm::mat4 getPerspectiveMatrix(const int screenWidth, const int screenHeight);
glm::mat4 getLookAtMatrix(glm::vec4 pos, glm::vec4 lookAt, glm::vec3 upVec);

} // namespace SirEngine