#pragma once
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include <vector>

namespace SirEngine {
namespace debug {
struct GraphStatus;
struct ShaderCompileConsole;

struct ShaderCompilerWidget final {
  ShaderCompilerWidget() : shaderName(""),offsetDevelopPath("../../"){};
  ~ShaderCompilerWidget();
  void initialize();
  void render();
  void log(const char* logValue);

  bool opened = 0;
  float width = 600;
  float height = 300;
  char shaderName[200];
  char offsetDevelopPath[200];
  std::vector<const char *> elementsToRender;
  int currentSelectedItem = 1;
  ShaderCompileConsole* console = nullptr;
  bool shouldRenderConsole = true;
  bool useDevelopPath = true;
  
};

} // namespace debug
} // namespace SirEngine
