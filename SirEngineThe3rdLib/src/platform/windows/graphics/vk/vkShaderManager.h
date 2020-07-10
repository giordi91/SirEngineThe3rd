#pragma once
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/memory/stackAllocator.h"
#include "platform/windows/graphics/vk/volk.h"
#include <cassert>
#include <d3dcommon.h>
#include <string>
#include <unordered_map>

namespace SirEngine::vk {
struct VkShaderArgs;
struct SpirVBlob;

class VkShaderCompiler;

struct VkShaderMetadata {
  SHADER_TYPE type;
  char *entryPoint;
  char *shaderPath;
  unsigned int shaderFlags;
  // NOTE: compiler args currently not supported
  // wchar_t *compilerArgs;
};
struct VkShaderBlob {
  VkShaderModule shader;
  VkShaderMetadata *metadata;
};

class VkShaderManager {
public:
  VkShaderManager() = default;
  ~VkShaderManager();
  // right now this is empty, is kept here for the time being
  // just for symmetry with the other managers
  void initialize();
  void loadShadersInFolder(const char *directory);
  void cleanup();

  inline VkShaderModule getShaderFromName(const std::string &name) {
    const auto found = m_stringToShader.find(name);
    if (found != m_stringToShader.end()) {
      return found->second.shader;
    }
    if (name == "null") {
      return nullptr;
    }
    assert(0 && "could not find shader");
    return nullptr;
  }

  inline const std::unordered_map<std::string, VkShaderBlob> &getShaderMap() {
    return m_stringToShader;
  }

  VkShaderManager(const VkShaderManager &) = delete;
  VkShaderManager &operator=(const VkShaderManager &) = delete;
  void loadShaderBinaryFile(const char *path);
  void recompileShader(const char *path, const char *offsetPath,
                       std::string *log);

private:
  // 2 mb of data for the stack
  const int METADATA_STACK_SIZE = 1 << 21;
  // data caching
  std::unordered_map<std::string, VkShaderBlob> m_stringToShader;
  StackAllocator m_metadataAllocator;
  VkShaderCompiler *m_compiler;
};
} // namespace SirEngine::vk
