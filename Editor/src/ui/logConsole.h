#pragma once

namespace Editor {
struct LogConsole;

enum class EditorLogLevel {
  LOG_NONE,
  LOG_WARNING,
  LOG_ERROR

};
struct LogConsoleWidget final {
  void initialize();
  void render();
  void log(const char* logValue, EditorLogLevel level=EditorLogLevel::LOG_NONE) const;

  bool m_opened = false;
  int m_currentSelectedItem = -1;
  LogConsole* m_console = nullptr;
  bool m_shouldRenderConsole = true;
};

}  // namespace Editor
