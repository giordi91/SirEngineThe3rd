#pragma once

namespace SirEngine {
class WindowResizeEvent;

class ImGuiManager {
 public:
  virtual void initialize() = 0;
  virtual void cleanup() = 0;
  virtual void startFrame() = 0;
  virtual void endFrame() = 0;
  virtual void onResizeEvent(const WindowResizeEvent &e) = 0;
};

}  // namespace SirEngine
