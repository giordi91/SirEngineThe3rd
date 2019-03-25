#pragma once
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/handle.h"

namespace SirEngine {

class RenderingContext {

public:
  RenderingContext() = default;
  ~RenderingContext() = default;
  void initialize();
  void setupCameraForFrame();
  void bindCameraBuffer(int index) const;
  inline void setEnviromentMap(const TextureHandle enviromentMapHandle) {
    m_enviromentMapHandle = enviromentMapHandle;
  };
  inline void setEnviromentMapIrradiance(
      const TextureHandle enviromentMapIrradianceHandle) {
    m_enviromentMapIrradianceHandle = enviromentMapIrradianceHandle;
  };
  inline TextureHandle getEnviromentMapHandle() const {
    return m_enviromentMapHandle;
  }
  inline TextureHandle getEnviromentMapIrradianceHandle() const {
    return m_enviromentMapIrradianceHandle;
  }

private:
  // member variable mostly temporary
  CameraBuffer m_camBufferCPU{};
  ConstantBufferHandle m_cameraHandle{};
  TextureHandle m_enviromentMapHandle;
  TextureHandle m_enviromentMapIrradianceHandle;
};

} // namespace SirEngine
