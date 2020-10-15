#pragma once

#include "vkAdapter.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/runtimeString.h"
#include "platform/windows/graphics/vk/volk.h"

namespace SirEngine {
namespace vk {
class VkBufferManager;
class VkMeshManager;
class VkPSOManager;
class VkShaderManager;
class VkTextureManager;
class VkPipelineLayoutManager;
class VkConstantBufferManager;
class VkBindingTableManager;
class VkDebugRenderer;
class VkCommandBufferManager;
struct VkSwapchain;

static constexpr int PREALLOCATED_SEMAPHORE_COUNT = 4;
static constexpr uint32_t VK_TIMEOUT_INFINITE =
    std::numeric_limits<uint32_t>::max();
struct VkFrameCommand final {
  // this might have to change for when we go multi-threaded
  VkCommandPool m_commandAllocator = nullptr;
  VkCommandBuffer m_commandBuffer = nullptr;
  bool m_isBufferOpen = false;
  VkFence m_endOfFrameFence = nullptr;
  VkSemaphore m_acquireSemaphore = nullptr;
  VkSemaphore m_renderSemaphore = nullptr;
  CommandBufferHandle handle = {};
};

// runtime instances
extern VkInstance INSTANCE;
extern VkSurfaceKHR SURFACE;
extern VkDevice LOGICAL_DEVICE;
extern VkQueue GRAPHICS_QUEUE;
extern VkQueue COMPUTE_QUEUE;
extern VkPhysicalDevice PHYSICAL_DEVICE;
extern VkFormat IMAGE_FORMAT;
extern VkDebugReportCallbackEXT DEBUG_CALLBACK;
extern VkDebugUtilsMessengerEXT DEBUG_CALLBACK2;
extern VkQueue PRESENTATION_QUEUE;
extern VkAdapterResult ADAPTER;

// defined by the engine
extern VkSwapchain *SWAP_CHAIN;
extern VkCommandBufferManager* COMMAND_BUFFER_MANAGER;
extern VkPSOManager *PSO_MANAGER;
extern VkShaderManager *SHADER_MANAGER;
extern VkPipelineLayoutManager *PIPELINE_LAYOUT_MANAGER;
extern VkConstantBufferManager *CONSTANT_BUFFER_MANAGER;
extern VkBufferManager *BUFFER_MANAGER;
extern VkMeshManager *MESH_MANAGER;
extern VkTextureManager *TEXTURE_MANAGER;
extern VkBindingTableManager *DESCRIPTOR_MANAGER;
extern VkDebugRenderer *DEBUG_RENDERER;
extern uint32_t SWAP_CHAIN_IMAGE_COUNT;
// incremented every frame and used to find the correct set of resources
// like command buffer pool and allocators
extern VkFrameCommand FRAME_COMMAND[PREALLOCATED_SEMAPHORE_COUNT];
extern VkFrameCommand *CURRENT_FRAME_COMMAND;
extern uint32_t GRAPHICS_QUEUE_FAMILY;
extern uint32_t PRESENTATION_QUEUE_FAMILY;
extern bool DEBUG_MARKERS_ENABLED;

inline VkImageLayout fromStateToLayout(RESOURCE_STATE state) {
  switch (state) {
    case RESOURCE_STATE::GENERIC:
      return VK_IMAGE_LAYOUT_GENERAL;
    case RESOURCE_STATE::RENDER_TARGET:
      return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RESOURCE_STATE::DEPTH_RENDER_TARGET:
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case RESOURCE_STATE::SHADER_READ_RESOURCE:
      return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case RESOURCE_STATE::RANDOM_WRITE:
      assert(0);  // need to figure out what to do with this
    default:
      assert(0);
  }
  return VK_IMAGE_LAYOUT_UNDEFINED;
}

inline uint32_t selectMemoryType(
    const VkPhysicalDeviceMemoryProperties &memoryProperties,
    const uint32_t memoryTypeBits, const VkMemoryPropertyFlags flags) {
  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
    uint32_t matchMemoryType = (memoryTypeBits & (1 << i)) != 0;
    uint32_t matchWantedFlags =
        (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags;
    if (matchMemoryType && (matchWantedFlags)) {
      return i;
    }
  }
  // assert(!"No compatible memory type found");
  return ~0u;
}

#define VK_CHECK(call)             \
  do {                             \
    VkResult result_ = call;       \
    assert(result_ == VK_SUCCESS); \
  } while (0)

#if _DEBUG
#define SET_DEBUG_NAME(resource, type, name)                                 \
  {                                                                          \
    VkDebugUtilsObjectNameInfoEXT debugInfo_{};                              \
    debugInfo_.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;   \
    debugInfo_.objectHandle = (uint64_t)resource;                            \
    debugInfo_.objectType = type;                                            \
    debugInfo_.pObjectName = SirEngine::persistentString(name);              \
    VK_CHECK(vkSetDebugUtilsObjectNameEXT(vk::LOGICAL_DEVICE, &debugInfo_)); \
  }
#else
#define SET_DEBUG_NAME(resource, type, name)
#endif

RenderingContext *createVkRenderingContext(
    const RenderingContextCreationSettings &settings, uint32_t width,
    uint32_t height);

class VkRenderingContext final : public RenderingContext {
  struct FrameBindingsData {
    VkRenderPass m_pass;
    VkFramebuffer *m_buffer;
    FrameBufferBindings m_bindings;
    uint32_t m_magicNumber;
    uint32_t m_frameBufferCount;
    const char *name;
  };

 public:
  explicit VkRenderingContext(const RenderingContextCreationSettings &settings,
                              uint32_t width, uint32_t height);
  ~VkRenderingContext() = default;
  // private copy and assignment
  VkRenderingContext(const VkRenderingContext &) = delete;
  VkRenderingContext &operator=(const VkRenderingContext &) = delete;

  bool initializeGraphics() override;

  void setupCameraForFrame() override;
  void bindCameraBuffer(RSHandle, bool isCompute = false) const override;

  bool newFrame() override;
  bool dispatchFrame() override;
  bool resize(uint32_t width, uint32_t height) override;
  bool stopGraphic() override;
  bool shutdownGraphic() override;
  void flush() override;
  void executeGlobalCommandList() override;
  void resetGlobalCommandList() override;
  void addRenderablesToQueue(const Renderable &renderable) override;

  void renderQueueType(const DrawCallConfig &config,
                       const SHADER_QUEUE_FLAGS flag,
                       const BindingTableHandle passBindings) override;
  BufferBindingsHandle prepareBindingObject(const FrameBufferBindings &bindings,
                                            const char *name) override;
  void setBindingObject(const BufferBindingsHandle handle) override;
  void clearBindingObject(const BufferBindingsHandle handle) override;
  void freeBindingObject(const BufferBindingsHandle handle) override;
  void fullScreenPass() override;

  void setViewportAndScissor(float offsetX, float offsetY, float width,
                             float height, float minDepth,
                             float maxDepth) override;
  void renderProcedural(const uint32_t indexCount) override;

  void dispatchCompute(uint32_t blockX, uint32_t blockY,
                       uint32_t blockZ) override;
  void renderProceduralIndirect(const BufferHandle &argsBuffer,
                                uint32_t offset = 0) override;
  void bindSamplers(const RSHandle &rs) override;

 private:
  inline void assertMagicNumber(const BufferBindingsHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(
               m_bindingsPool.getConstRef(idx).m_magicNumber) == magic &&
           "invalid magic handle for constant buffer");
  }

 private:
  void *queues = nullptr;
  ConstantBufferHandle m_cameraHandle{};
  FrameData m_frameData{};
  static constexpr uint32_t RESERVE_SIZE = 400;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  SparseMemoryPool<FrameBindingsData> m_bindingsPool;
  uint32_t m_matrixCounter = 0;
  BufferHandle m_matrixBufferHandle[5]{};
};

}  // namespace vk
}  // namespace SirEngine
