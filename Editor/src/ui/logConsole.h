#pragma once

namespace Editor{
struct LogConsole;

struct LogConsoleWidget final {
  void initialize();
  void render();
  void log(const char* logValue);

  bool m_opened = false;
  int m_currentSelectedItem = -1;
  LogConsole* m_console = nullptr;
  bool m_shouldRenderConsole = true;
};

}  // namespace Editor::ui
