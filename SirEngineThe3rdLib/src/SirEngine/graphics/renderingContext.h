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
  void setupLightingForFrame();
  void bindCameraBuffer(int index) const;
  void bindCameraBufferCompute(int index) const;
  void updateSceneBoundingBox();
  void updateDirectionalLightMatrix();

  inline void setEnviromentMap(const TextureHandle enviromentMapHandle) {
    m_enviromentMapHandle = enviromentMapHandle;
  }

  inline void setEnviromentMapIrradiance(
      const TextureHandle enviromentMapIrradianceHandle) {
    m_enviromentMapIrradianceHandle = enviromentMapIrradianceHandle;
  }

  inline const DirectionalLightData &getLightData() const { return m_light; };
  inline void
  setEnviromentMapRadiance(const TextureHandle enviromentMapRadianceHandle) {
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
  inline void setBrdfHandle(const TextureHandle handle) {
    m_brdfHandle = handle;
  }
  inline TextureHandle getBrdfHandle() const { return m_brdfHandle; }

  inline ConstantBufferHandle getLightCB() const { return m_lightCB; }
  inline BoundingBox getBoundingBox() const { return m_boundingBox; }

private:
  // member variable mostly temporary
  CameraBuffer m_camBufferCPU{};
  ConstantBufferHandle m_cameraHandle{};
  ConstantBufferHandle m_lightBuffer{};
  ConstantBufferHandle m_lightCB{};
  DirectionalLightData m_light;
  TextureHandle m_enviromentMapHandle;
  TextureHandle m_enviromentMapIrradianceHandle;
  TextureHandle m_enviromentMapRadianceHandle;
  TextureHandle m_brdfHandle;
  BoundingBox m_boundingBox;
  DebugDrawHandle m_lightAABBHandle{};
};

} // namespace SirEngine
