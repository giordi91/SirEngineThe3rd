#pragma once
#include "SirEngine/core.h"
#include "vulkan/vulkan.h"
#include <string>

namespace SirEngine::vk {

struct SIR_ENGINE_API VkShaderArgs {
  VkShaderArgs() : entryPoint(nullptr), type(nullptr), compilerArgs(nullptr) {}
  // splitCompilerArgsPointers(20){};
  bool debug = false;

  wchar_t *entryPoint;
  wchar_t *type;
  wchar_t *compilerArgs;
  // this vector instead will hold the point of the args
  // ResizableVector<wchar_t *> splitCompilerArgsPointers;
};

class SIR_ENGINE_API VkShaderCompiler {
public:
  VkShaderCompiler();
  ~VkShaderCompiler();
  VkShaderModule compileShader(const char *shaderPath, VkShaderArgs &shaderArgs,
                               std::string *log);
  unsigned int getShaderFlags(const VkShaderArgs &shaderArgs);
};

} // namespace SirEngine::vk
