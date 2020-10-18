#pragma once

#include "SirEngine/ui/imguiManager.h"

namespace SirEngine::dx12 {

class Dx12ImGuiManager final : public ImGuiManager {
 public:
  void initialize() override;
  void cleanup() override;
  void startFrame() override;
  void endFrame() override;

  void onResizeEvent(const WindowResizeEvent& e) override;
};
}  // namespace SirEngine::vk
