#include "SirEngine/graphics/lightManager.h"

namespace SirEngine::graphics {
LightHandle LightManager::addLight(const DirectionalLightData data) {
  m_directionalLights.pushBack(data);
  // generate the handle
  return generateHandle(LIGHT_TYPE::DIRECTIONAL,
                        m_directionalLights.size() - 1);
}

}  // namespace SirEngine::graphics
