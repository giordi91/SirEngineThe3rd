#pragma once
#include "platform/windows/graphics/vk/volk.h"
#include <vulkan/vulkan.h>

#include "SirEngine/graphics/renderingContext.h"
#include <cassert>
#include <vector>

namespace SirEngine {
namespace vk {
class VkPSOManager;
class VkShaderManager;
struct VkSwapchain;

// runtime instances
extern VkInstance INSTANCE;
extern VkSurfaceKHR SURFACE;
extern VkDevice LOGICAL_DEVICE;
extern VkQueue GRAPHICS_QUEUE;
extern VkQueue COMPUTE_QUEUE;
extern VkPhysicalDevice PHYSICAL_DEVICE;
extern VkRenderPass RENDER_PASS;
extern VkSemaphore IMAGE_ACQUIRED_SEMAPHORE;
extern VkSemaphore READY_TO_PRESENT_SEMAPHORE;
extern VkCommandPool COMMAND_POOL;
extern VkCommandBuffer COMMAND_BUFFER;
extern VkFormat IMAGE_FORMAT;
extern VkPipelineLayout PIPELINE_LAYOUT;
extern VkDebugReportCallbackEXT DEBUG_CALLBACK;
extern VkDebugUtilsMessengerEXT DEBUG_CALLBACK2;
extern VkQueue PRESENTATION_QUEUE;

// defined by the engine
extern VkSwapchain *SWAP_CHAIN;
extern VkPSOManager *PSO_MANAGER;
extern VkShaderManager *SHADER_MANAGER;

extern std::vector<VkDescriptorSetLayout> LAYOUTS_TO_DELETE;

bool vkInitializeGraphics(BaseWindow *wnd, const uint32_t width,
                          const uint32_t height);
bool onResize(uint32_t width, uint32_t height);

#define VK_CHECK(call)                                                         \
  do {                                                                         \
    VkResult result_ = call;                                                   \
    assert(result_ == VK_SUCCESS);                                             \
  } while (0)

#ifndef ARRAYSIZE
#define ARRAYSIZE(array) sizeof(array) / sizeof(array[0]);
#endif

#if _DEBUG
#define SET_DEBUG_NAME(resource, type, name)                                   \
  {                                                                            \
    VkDebugUtilsObjectNameInfoEXT debugInfo_{};                                \
    debugInfo_.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;     \
    debugInfo_.objectHandle = (uint64_t)resource;                              \
    debugInfo_.objectType = type;                                              \
    debugInfo_.pObjectName = name;                                             \
    vkSetDebugUtilsObjectNameEXT(vk::LOGICAL_DEVICE, &debugInfo_);             \
  }
#else
SET_DEBUG_NAME(resource, type, name)
#endif

RenderingContext *
createVkRenderingContext(const RenderingContextCreationSettings &settings,
                         uint32_t width, uint32_t height);

class VkRenderingContext final : public RenderingContext {
public:
  explicit VkRenderingContext(const RenderingContextCreationSettings &settings,
                              uint32_t width, uint32_t height);
  ~VkRenderingContext() = default;
  // private copy and assignment
  VkRenderingContext(const VkRenderingContext &) = delete;
  VkRenderingContext &operator=(const VkRenderingContext &) = delete;

  bool initializeGraphics() override;

  void setupCameraForFrame();
  void setupLightingForFrame();
  void bindCameraBuffer(int index) const;
  void bindCameraBufferCompute(int index) const;
  void updateSceneBoundingBox();
  void updateDirectionalLightMatrix();

  /*
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

  */
  bool newFrame() override;
  bool dispatchFrame() override;
  bool resize(uint32_t width, uint32_t height) override;
  bool stopGraphic() override;
  bool shutdownGraphic() override;
  void flush() override;
  void executeGlobalCommandList() override;
  void resetGlobalCommandList() override;

private:
  /*
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
*/
};

} // namespace vk
} // namespace SirEngine
