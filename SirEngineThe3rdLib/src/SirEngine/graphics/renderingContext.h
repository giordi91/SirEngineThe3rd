#pragma once
#include "SirEngine/engineConfig.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/handle.h"

namespace SirEngine {

class BaseWindow;
/*
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
*/
struct APIConfig {
  bool vsync = false;
  uint32_t apiVersion = 0;
  // here you can put per api flags
  uint32_t genericApiFlags = 0;
};

struct RenderingContextCreationSettings {
  GRAPHIC_API graphicsAPI;
  APIConfig apiConfig;
  BaseWindow *window;
  uint32_t inFlightFrames = 2;
  uint32_t width;
  uint32_t height;
  bool isHeadless = false;
};

struct ScreenInfo {
  uint32_t width;
  uint32_t height;
};

class RenderingContext {

public:
  virtual ~RenderingContext() = default;
  // private copy and assignment
  RenderingContext(const RenderingContext &) = delete;
  RenderingContext &operator=(const RenderingContext &) = delete;

  static RenderingContext *
  create(const RenderingContextCreationSettings &settings, uint32_t width,
         uint32_t height);
  static bool isAPISupported(const GRAPHIC_API graphicsAPI);
  virtual bool initializeGraphics() = 0;
  virtual bool newFrame() = 0;
  virtual bool dispatchFrame() = 0;
  virtual bool resize(uint32_t width, uint32_t height) = 0;
  virtual bool stopGraphic() = 0;
  virtual bool shutdownGraphic() = 0;
  virtual void flush() = 0;
  virtual void executeGlobalCommandList() = 0;
  virtual void resetGlobalCommandList() = 0;

  inline const RenderingContextCreationSettings &getContextSettings() const {
    return m_settings;
  }
  inline ScreenInfo getScreenInfo() const { return m_screenInfo; }

protected:
  // Anonymous parameters are width and height, removed the name to mute the
  // visual studio warning
  explicit RenderingContext(const RenderingContextCreationSettings &settings,
                            uint32_t width, uint32_t height)
      : m_settings(settings) {
    m_screenInfo = {width, height};
  };
  RenderingContextCreationSettings m_settings;
  ScreenInfo m_screenInfo;
};

} // namespace SirEngine
