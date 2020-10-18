#pragma once
#include <vulkan/vulkan_core.h>

#include "SirEngine/ui/imguiManager.h"

namespace SirEngine::vk {

class VkImGuiManager final : public ImGuiManager {
 public:
  void initialize() override;
  void cleanup() override;
  void startFrame() override;
  void endFrame() override;

  void onResizeEvent(const WindowResizeEvent& e) override;
 private:
  VkRenderPass imguiPass{};
};
}  // namespace SirEngine::vk
