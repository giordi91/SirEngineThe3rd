#pragma once
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/cpu/resizableVector.h"

namespace SirEngine::graphics {

enum class LIGHT_TYPE {
  UNDEFINED = 0,
  DIRECTIONAL = 1,
};

class LightManager {
  // in the handle we use 12 bits for light index 12 bits for magic control
  // number finally  the last 8 bits are for the light type
  static constexpr uint32_t RESERVE_SIZE = 10;
  static constexpr uint32_t MAX_LIGHT_COUNT = 1 << 12;
  static constexpr uint32_t LIGHT_BIT_COUNT = 12;
  static constexpr uint32_t MAGIC_NUMBER_OFFSET = LIGHT_BIT_COUNT;
  static constexpr uint32_t MAGIC_NUMBER_BIT_COUNT = 12;
  static constexpr uint32_t TYPE_OFFSET =
      MAGIC_NUMBER_BIT_COUNT + LIGHT_BIT_COUNT;
  static constexpr uint32_t TYPE_SIZE = 8;

 public:
  enum LIGHT_CREATION_FLAGS { NONE = 0, GPU_BACKED = 1 };

  LightManager() : m_directionalLights(RESERVE_SIZE) {}
  LightHandle addLight(DirectionalLightData data,
                       const LIGHT_CREATION_FLAGS flags);
  void cleanup(){}

  [[nodiscard]] static LIGHT_TYPE getLightType(const LightHandle handle) {
    uint32_t raw = handle.handle;
    auto mask = static_cast<uint32_t>((1 << TYPE_SIZE) - 1);
    uint32_t typeRaw = (raw >> TYPE_OFFSET) & mask;
    return static_cast<LIGHT_TYPE>(typeRaw);
  }
  [[nodiscard]] ConstantBufferHandle getLightBuffer(const LightHandle handle) {
    uint32_t index = getLightIndex(handle);
    uint32_t magic = getLightMagicNumber(handle);

    assert(index <= m_directionalLights.size());
    const DirectionalLightDataTracker& lightData =
        m_directionalLights.getConstRef(index);
    assert(lightData.m_magicNumber == magic);
    assert(isFlagSet(lightData.m_flags, GPU_BACKED));

    return lightData.m_cbhandle;
  }

 private:
  struct DirectionalLightDataTracker {
    DirectionalLightData m_data;
    uint32_t m_flags;
    ConstantBufferHandle m_cbhandle;
    uint32_t m_magicNumber;
  };

 private:
  LightHandle generateHandle(const LIGHT_TYPE type, const uint32_t id) {
    uint32_t typeShift = (static_cast<uint32_t>(type) << TYPE_OFFSET);
    uint32_t magicNumberShift = (MAGIC_NUMBER_COUNTER << MAGIC_NUMBER_OFFSET);
    const LightHandle handle{typeShift | magicNumberShift | id};
    ++MAGIC_NUMBER_COUNTER;
    return handle;
  }
  static uint32_t getLightMagicNumber(const LightHandle handle) {
    // first we generate the mask we care about, then we shift it by the size
    // of the bit count
    uint32_t mask = ((1 << MAGIC_NUMBER_BIT_COUNT) - 1) << LIGHT_BIT_COUNT;
    // we shift back after masking
    return (handle.handle & mask) >> LIGHT_BIT_COUNT;
  }

  inline bool isFlagSet(const uint32_t flags,
                        const LightManager::LIGHT_CREATION_FLAGS toCheck) {
    return (flags & toCheck) > 0;
  }

  static uint32_t getLightIndex(const LightHandle handle) {
    return handle.handle & ((1 << LIGHT_BIT_COUNT) - 1);
  }

 private:
  ResizableVector<DirectionalLightDataTracker> m_directionalLights;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};

}  // namespace SirEngine::graphics
