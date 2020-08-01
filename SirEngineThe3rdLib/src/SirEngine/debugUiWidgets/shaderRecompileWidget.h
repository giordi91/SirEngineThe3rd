#pragma once
#include <string>
#include <vector>

namespace SirEngine::debug {
struct GraphStatus;
struct ShaderCompileConsole;

struct ShaderCompilerWidget final {
  ShaderCompilerWidget() : shaderName(""), offsetDevelopPath("../../"){};
  ~ShaderCompilerWidget();
  void initialize();
  void render();
  void log(const char* logValue);
  void requestCompile();

  bool m_opened = false;
  float m_width = 600;
  float m_height = 300;
  char shaderName[200];
  char offsetDevelopPath[200];
  std::vector<const char*> m_elementsToRender;
  int m_currentSelectedItem = -1;
  ShaderCompileConsole* m_console = nullptr;
  bool m_shouldRenderConsole = true;
  bool m_useDevelopPath = true;

  std::string m_currentSelectedShader;
};

}  // namespace SirEngine
