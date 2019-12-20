#pragma once
#include "SirEngine/engineConfig.h"
#include "SirEngine/handle.h"
#include <glm/mat4x4.hpp>

namespace SirEngine {

class BaseWindow;

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
  virtual void renderQueueType(const SHADER_QUEUE_FLAGS flag) = 0;
  virtual void renderMaterialType(const SHADER_QUEUE_FLAGS flag) = 0;

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
    m_screenInfo.width = width;
    m_screenInfo.height = height;
  };
  RenderingContextCreationSettings m_settings;
  ScreenInfo m_screenInfo;
};

} // namespace SirEngine
