#pragma once
#include <vulkan/vulkan.h>

#include "SirEngine/handle.h"
#include "SirEngine/layer.h"
#include "platform/windows/graphics/vk/vkMeshManager.h"
#include "platform/windows/graphics/vk/vkTexture.h"
#include "SirEngine/graphics/nodes/vkSimpleForward.h"

namespace SirEngine {
namespace vk {
struct VkMesh;
struct VkTexture2D;
} // namespace vk

class ReloadScriptsEvent;
class DebugRenderConfigChanged;
class MouseButtonPressEvent;
class MouseButtonReleaseEvent;
class MouseMoveEvent;
class WindowResizeEvent;
class DebugLayerChanged;
class ShaderCompileEvent;
struct Skeleton;
struct GraphAllocators;

namespace dx12 {
class Texture2D;

} // namespace dx12
class SIR_ENGINE_API VkTempLayer final : public Layer {
public:
  VkTempLayer() : Layer("VkTempLayer") {}
  ~VkTempLayer() override = default;

  void onAttach() override;
  void onDetach() override;
  void onUpdate() override;
  void onEvent(Event &event) override;
  void clear() override;

private:
  // event implementation for the layer
  bool onMouseButtonPressEvent(MouseButtonPressEvent &e);
  bool onMouseButtonReleaseEvent(MouseButtonReleaseEvent &e);
  bool onMouseMoveEvent(MouseMoveEvent &e);
  bool onDebugLayerEvent(DebugLayerChanged &e);
  bool onResizeEvent(WindowResizeEvent &e);
  bool onDebugConfigChanged(DebugRenderConfigChanged &e);
  bool onShaderCompileEvent(ShaderCompileEvent &e);
  bool onReloadScriptEvent(ReloadScriptsEvent &e);
  void setupCameraForFrame();

  // camera event control
  bool leftDown = false;
  bool rightDown = false;
  bool middleDown = false;
  float previousX = 0;
  float previousY = 0;

  void createDescriptorLayoutAdvanced();
  void createRenderTargetAndFrameBuffer(int width, int height);
  // shaders
  VkPipeline m_pipeline;

  // mesh
  MeshHandle meshHandle;
  TextureHandle textureHandle;
  CameraBuffer m_camBufferCPU;
  ConstantBufferHandle m_cameraBufferHandle;

  VkVertexInputBindingDescription m_stream;
  VkVertexInputAttributeDescription m_attr[3];
  VkDescriptorSetLayout m_setLayout;
  VkDescriptorSet m_meshDescriptorSet;
  vk::VkTexture2D m_rt;
  VkFramebuffer m_tempFrameBuffer;
  VkRenderPass m_pass;
  VkSimpleForward* m_forward;
  // TODO temp
  GraphAllocators *alloc;
};
} // namespace SirEngine
