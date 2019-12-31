#pragma once
#include "SirEngine/engineConfig.h"
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/handle.h"
#include <glm/mat4x4.hpp>

namespace SirEngine {

class BaseWindow;

enum class RESOURCE_STATE {
  GENERIC,
  RENDER_TARGET,
  DEPTH_RENDER_TARGET,
  SHADER_READ_RESOURCE,
  RANDOM_WRITE
};

struct RTBinding {
  TextureHandle handle{};
  glm::vec4 clearColor{0, 0, 0, 1};
  RESOURCE_STATE currentResourceState;
  RESOURCE_STATE neededResourceState;
  uint32_t shouldClearColor : 1;
  uint32_t padding : 31;
};
struct DepthBinding {
  TextureHandle handle{};
  glm::vec4 clearDepthColor{0, 0, 0, 1};
  glm::vec4 clearStencilColor{0, 0, 0, 1};
  RESOURCE_STATE currentResourceState;
  RESOURCE_STATE neededResourceState;
  uint32_t shouldClearDepth : 1;
  uint32_t shouldClearStencil : 1;
  uint32_t padding : 30;
};
struct FrameBufferBindings {
  RTBinding colorRT[8] = {};
  DepthBinding depthStencil = {};
  RTBinding *extraBindings = nullptr;
  uint32_t extraBindingsCount = 0;
  uint32_t width = 0;
  uint32_t height = 0;
};

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

struct Renderable {
  glm::mat4 m_matrixRuntime;
  MeshHandle m_meshHandle;
  MaterialHandle m_materialHandle;
};

enum class DRAW_CALL_FLAGS {
  SHOULD_CLEAR_COLOR = 1,
  SHOULD_CLEAR_DEPTH_STENCIL = 2
};

struct DrawCallConfig {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t flags = 0;
  glm::vec4 clearColorValue;
  glm::vec4 clearDepthValue;
  glm::vec4 clearStencilValue;
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
  virtual void addRenderablesToQueue(const Renderable &renderable) = 0;
  virtual void renderQueueType(const DrawCallConfig &config,
                               const SHADER_QUEUE_FLAGS flag) = 0;
  virtual void renderMaterialType(const SHADER_QUEUE_FLAGS flag) = 0;
  // simply submit a full screen quad to the render pipeline
  virtual void fullScreenPass() = 0;

  virtual void setupCameraForFrame() = 0;
  virtual BufferBindingsHandle
  prepareBindingObject(const FrameBufferBindings &bindings,
                       const char *name) = 0;
  virtual void setBindingObject(const BufferBindingsHandle handle) = 0;
  virtual void clearBindingObject(const BufferBindingsHandle handle) = 0;
  virtual void freeBindingObject(const BufferBindingsHandle handle) = 0;
  virtual void bindCameraBuffer(int index) const = 0;

  inline const RenderingContextCreationSettings &getContextSettings() const {
    return m_settings;
  }
  inline ScreenInfo getScreenInfo() const { return m_screenInfo; }

  // TODO texture setter and getter, need to find a better way to do it
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

protected:
  // Anonymous parameters are width and height, removed the name to mute the
  // visual studio warning
  explicit RenderingContext(const RenderingContextCreationSettings &settings,
                            uint32_t width, uint32_t height)
      : m_settings(settings) {
    m_screenInfo = {width, height};
    m_screenInfo.width = width;
    m_screenInfo.height = height;
  };

protected:
  RenderingContextCreationSettings m_settings;
  ScreenInfo m_screenInfo{};
  TextureHandle m_enviromentMapHandle{};
  TextureHandle m_enviromentMapIrradianceHandle{};
  TextureHandle m_enviromentMapRadianceHandle{};
  TextureHandle m_brdfHandle{};
  BoundingBox m_boundingBox{};
  ConstantBufferHandle m_lightCB{};
  DirectionalLightData m_light{};
};

} // namespace SirEngine
