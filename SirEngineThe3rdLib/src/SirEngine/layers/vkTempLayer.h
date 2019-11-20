#pragma once
#include <vulkan/vulkan.h>

#include "SirEngine/handle.h"
#include "SirEngine/layer.h"
#include "platform/windows/graphics/vk/vkMemory.h"
#include "platform/windows/graphics/vk/vkMesh.h"
#include "platform/windows/graphics/vk/vkTexture.h"

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
  VkTempLayer() : Layer("DX12GraphicsLayer") {}
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

  // camera event control
  bool leftDown = false;
  bool rightDown = false;
  bool middleDown = false;
  float previousX = 0;
  float previousY = 0;

  void createDescriptorLayoutAdvanced();
  // shaders
  VkShaderModule m_vs;
  VkShaderModule m_fs;
  VkPipeline m_pipeline;

  // mesh
  vk::VkMesh m_mesh;
  vk::Buffer m_vertexBuffer;
  vk::Buffer m_indexBuffer;
  VkVertexInputBindingDescription m_stream;
  VkVertexInputAttributeDescription m_attr[3];
  VkDescriptorPool m_dPool;
  VkDescriptorSetLayout m_setLayout;
  VkDescriptorSet m_meshDescriptorSet;
  vk::VkTexture2D uvTexture;
};
} // namespace SirEngine
