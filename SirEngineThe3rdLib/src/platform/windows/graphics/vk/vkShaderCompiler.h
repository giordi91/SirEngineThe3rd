#pragma once
#include <string>
#include <vector>

#include "SirEngine/graphics/graphicsDefines.h"
#include "vulkan/vulkan.h"

namespace SirEngine::vk {

struct VkShaderArgs {
  bool debug = false;
  SHADER_TYPE type = SHADER_TYPE::INVALID;
};

class VkShaderCompiler {
 public:
  VkShaderCompiler();
  ~VkShaderCompiler();
  SpirVBlob compileToSpirV(const char *shaderPath, VkShaderArgs &shaderArgs,
                           std::string *log) const;
  std::string compileToHlsl(const char *shaderPath, VkShaderArgs &shaderArgs,
                            std::string *log) const;
  std::string sprivToGlsl(const std::vector<unsigned int> &spirV);
  static VkShaderModule spirvToShaderModule(const SpirVBlob &blob);
  VkShaderModule compileToShaderModule(const char *shaderPath,
                                       VkShaderArgs &shaderArgs,
                                       std::string *log) const;
  static uint32_t getShaderFlags(const VkShaderArgs &shaderArgs);
  static SHADER_TYPE getShaderTypeFromName(const char *name);
};

}  // namespace SirEngine::vk
