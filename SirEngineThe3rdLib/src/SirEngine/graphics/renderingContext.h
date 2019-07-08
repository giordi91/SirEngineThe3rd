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
  void bindCameraBufferCompute(int index) const;
  inline void setEnviromentMap(const TextureHandle enviromentMapHandle) {
    m_enviromentMapHandle = enviromentMapHandle;
  };
  inline void setEnviromentMapIrradiance(
      const TextureHandle enviromentMapIrradianceHandle) {
    m_enviromentMapIrradianceHandle = enviromentMapIrradianceHandle;
  };
  inline void setEnviromentMapRadiance(
      const TextureHandle enviromentMapRadianceHandle) {
    m_enviromentMapRadianceHandle = enviromentMapRadianceHandle;
  };
  inline TextureHandle getEnviromentMapHandle() const {
    return m_enviromentMapHandle;
  }
  inline TextureHandle getEnviromentMapIrradianceHandle() const {
    return m_enviromentMapIrradianceHandle;
  }
  inline TextureHandle getEnviromentMapRadianceHandle() const {
    return m_enviromentMapRadianceHandle;
  }

private:
  // member variable mostly temporary
  CameraBuffer m_camBufferCPU{};
  ConstantBufferHandle m_cameraHandle{};
  TextureHandle m_enviromentMapHandle;
  TextureHandle m_enviromentMapIrradianceHandle;
  TextureHandle m_enviromentMapRadianceHandle;
};

} // namespace SirEngine
