#pragma once


#include "SirEngine/handle.h"

#include <imgui/imgui.h>

namespace SirEngine {
class WindowResizeEvent;

class ImGuiManager {
 public:
	virtual ~ImGuiManager() = default;
	virtual void initialize() = 0;
  virtual void cleanup() = 0;
  virtual void startFrame() = 0;
  virtual void endFrame() = 0;
  virtual void onResizeEvent(const WindowResizeEvent &e) = 0;
  virtual ImTextureID getImguiImageHandle(const TextureHandle& handle) = 0;
};

}  // namespace SirEngine
