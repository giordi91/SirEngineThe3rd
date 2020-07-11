#pragma once
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/resizableVector.h"

namespace SirEngine::graphics {

enum class LIGHT_TYPE {
  UNDEFINED = 0,
  DIRECTIONAL = 1,
};

class LightManager {
  static constexpr uint32_t RESERVE_SIZE = 10;
  static constexpr uint32_t MAX_LIGHT_COUNT = 1 << 12;

 public:
  LightManager() : m_directionalLights(RESERVE_SIZE) {}
  LightHandle addLight(DirectionalLightData data);

 private:
  static LightHandle generateHandle(const LIGHT_TYPE type, const uint32_t id) {
    const LightHandle handle{(id << MAX_LIGHT_COUNT) | id};
    return handle;
  }

 private:
  ResizableVector<DirectionalLightData> m_directionalLights;
};

}  // namespace SirEngine::graphics
