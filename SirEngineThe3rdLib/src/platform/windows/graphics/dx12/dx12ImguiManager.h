#pragma once

#include "SirEngine/ui/imguiManager.h"

#include <imgui/imgui.h>

namespace SirEngine {
struct TextureHandle;
}

namespace SirEngine::dx12 {

class Dx12ImGuiManager final : public ImGuiManager {
 public:
  ~Dx12ImGuiManager() override = default;
  void initialize() override;
  void cleanup() override;
  void startFrame() override;
  void endFrame() override;

  void onResizeEvent(const WindowResizeEvent& e) override;
  ImTextureID getImguiImageHandle(const TextureHandle& handle) override;
};
}  // namespace SirEngine::dx12
