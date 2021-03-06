#pragma once
#include <glm/mat4x4.hpp>

#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/handle.h"
#include "SirEngine/graphics/graphicsDefines.h"

namespace SirEngine {

class BaseWindow;


struct RTBinding {
  TextureHandle handle{};
  glm::vec4 clearColor{0, 0, 0, 1};
  RESOURCE_STATE currentResourceState;
  RESOURCE_STATE neededResourceState;
  uint32_t shouldClearColor : 1;
  uint32_t isSwapChainBackBuffer : 1;
  uint32_t padding : 30;
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

struct RenderableDescription {
  // index buffer can be null
  BufferHandle indexBuffer{};
  BufferHandle buffer{};
  MemoryRange subranges[6]{};
  uint32_t subragesCount = 0;
  uint32_t primitiveToRender = 0;
  MaterialHandle materialHandle;
};

enum class DRAW_CALL_FLAGS {
  SHOULD_CLEAR_COLOR = 1,
  SHOULD_CLEAR_DEPTH_STENCIL = 2
};

struct DrawCallConfig {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t flags = 0;
};

class RenderingContext {
 public:
  virtual ~RenderingContext() = default;
  virtual void bindSamplers(const RSHandle & rs) = 0;
  // private copy and assignment
  RenderingContext(const RenderingContext &) = delete;
  RenderingContext &operator=(const RenderingContext &) = delete;

  static RenderingContext *create(
      const RenderingContextCreationSettings &settings, uint32_t width,
      uint32_t height);
  static bool isApiSupported(const GRAPHIC_API graphicsApi);
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
                               const SHADER_QUEUE_FLAGS flag,
                               BindingTableHandle passBindings) = 0;
  virtual void renderProcedural(const uint32_t indexCount) = 0;
  virtual void renderProceduralIndirect(const BufferHandle &argsBuffer, uint32_t offset = 0) = 0;
  virtual void setViewportAndScissor(float offsetX, float offsetY, float width,
                                     float height, float minDepth,
                                     float maxDepth) = 0;
  // simply submit a full screen quad to the render pipeline
  virtual void fullScreenPass() = 0;

  virtual void setupCameraForFrame(uint32_t renderWidth, uint32_t renderHeight, bool updateCameraMovment) = 0;
  virtual BufferBindingsHandle prepareBindingObject(
      const FrameBufferBindings &bindings, const char *name) = 0;
  virtual void setBindingObject(const BufferBindingsHandle handle) = 0;
  virtual void clearBindingObject(const BufferBindingsHandle handle) = 0;
  virtual void freeBindingObject(const BufferBindingsHandle handle) = 0;
  virtual void bindCameraBuffer(RSHandle, bool compute = false) const = 0;
  virtual void dispatchCompute(uint32_t blockX, uint32_t blockY,
                               uint32_t blockW) = 0;

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

}  // namespace SirEngine
