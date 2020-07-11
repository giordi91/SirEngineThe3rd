#include "SirEngine/graphics/lightManager.h"

#include "SirEngine/constantBufferManager.h"
#include "SirEngine/globals.h"

namespace SirEngine::graphics {

LightHandle LightManager::addLight(DirectionalLightData data,
                                   const LIGHT_CREATION_FLAGS flags) {
  DirectionalLightDataTracker tracker{
      data, static_cast<uint32_t>(flags), {}, 0};

  // check the flags
  if (isFlagSet(flags, LIGHT_CREATION_FLAGS::GPU_BACKED)) {
    // we need to allocate the constant buffer
    // allocate the constant buffer
    ConstantBufferHandle cbhandle = globals::CONSTANT_BUFFER_MANAGER->allocate(
        sizeof(DirectionalLightData), 0, &data);
    tracker.m_cbhandle = cbhandle;
  }

  // generate the handle
  LightHandle handle =
      generateHandle(LIGHT_TYPE::DIRECTIONAL, m_directionalLights.size());
  // lets get out the magic number used for this handle;
  tracker.m_magicNumber = getLightMagicNumber(handle);

  m_directionalLights.pushBack(tracker);
  return handle;
}

}  // namespace SirEngine::graphics
