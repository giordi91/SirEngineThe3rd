#pragma once


#include "SirEngine/handle.h"
#include "SirEngine/ui/imguiManager.h"

#include <imgui/imgui.h>
#include <vulkan/vulkan_core.h>

namespace SirEngine::vk {

class VkImGuiManager final : public ImGuiManager {
 public:
  ~VkImGuiManager() override = default;
  void initialize() override;
  void cleanup() override;
  void startFrame() override;
  void endFrame() override;

  void onResizeEvent(const WindowResizeEvent& e) override;
  ImTextureID getImguiImageHandle(const TextureHandle& handle) override;

 private:
  VkRenderPass imguiPass{};
};
}  // namespace SirEngine::vk
