#pragma once
#include "SirEngine/core.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "vulkan/vulkan.h"
#include <string>

namespace SirEngine::vk {
struct SIR_ENGINE_API SpirVBlob {
  void *memory = nullptr;
  uint32_t sizeInByte = 0;
};

struct SIR_ENGINE_API VkShaderArgs {
  bool debug = false;
  SHADER_TYPE type = SHADER_TYPE::INVALID;
};

class SIR_ENGINE_API VkShaderCompiler {
public:
  VkShaderCompiler();
  ~VkShaderCompiler();
  SpirVBlob compileToSpirV(const char *shaderPath, VkShaderArgs &shaderArgs,
                           std::string *log) const;
  std::string compileToHlsl(const char *shaderPath, VkShaderArgs &shaderArgs,
                           std::string *log) const;
  static VkShaderModule spirvToShaderModule(const SpirVBlob& blob);
  VkShaderModule compileToShaderModule(const char *shaderPath,
                                       VkShaderArgs &shaderArgs,
                                       std::string *log) const;
  static uint32_t getShaderFlags(const VkShaderArgs &shaderArgs);
  static SHADER_TYPE getShaderTypeFromName(const char*name);
};

} // namespace SirEngine::vk
