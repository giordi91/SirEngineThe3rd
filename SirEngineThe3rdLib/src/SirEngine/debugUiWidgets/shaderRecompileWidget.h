#pragma once
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include <vector>

namespace SirEngine {
namespace debug {
struct GraphStatus;
struct ShaderCompileConsole;

struct ShaderCompilerWidget final {
  ShaderCompilerWidget() : shaderName(""){};
  ~ShaderCompilerWidget();
  void initialize();
  void render();
  void log(const char* logValue);

  bool opened = 0;
  float width = 600;
  float height = 300;
  char shaderName[200];
  std::vector<const char *> elementsToRender;
  int currentSelectedItem = 1;
  ShaderCompileConsole* console = nullptr;
  bool shouldRenderConsole = true;
  
};

} // namespace debug
} // namespace SirEngine
