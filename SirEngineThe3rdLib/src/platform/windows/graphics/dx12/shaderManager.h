#pragma once
#include "SirEngine/memory/stackAllocator.h"
#include <cassert>
#include <d3dcommon.h>
#include <string>
#include <unordered_map>

namespace SirEngine {
namespace dx12 {

struct ShaderMetadata {
  wchar_t *type;
  wchar_t *entryPoint;
  char *shaderPath;
  unsigned int shaderFlags;
};
struct ShaderBlob {
  ID3DBlob *shader;
  ShaderMetadata *metadata;
};

class ShaderManager {

public:
  ~ShaderManager() = default;
  // right now this is empty, is kept here for the time being
  // just for simmetry with the other managers
  void init();
  void loadShadersInFolder(const char *directory);
  void cleanup();
  inline ID3DBlob *getShaderFromName(const std::string &name) {
    auto found = m_stringToShader.find(name);
    if (found != m_stringToShader.end()) {
      return found->second.shader;
    }
    assert(0 && "could not find shader");
    return nullptr;
  }

  ShaderManager() = default;
  ShaderManager(const ShaderManager &) = delete;
  ShaderManager &operator=(const ShaderManager &) = delete;
  void loadShaderFile(const char *path);
  void loadShaderBinaryFile(const char *path);

private:
  // 2 mb of data for the stack 
  const int METADATA_STACK_SIZE = 1<<21;
  // data caching
  std::unordered_map<std::string, ShaderBlob> m_stringToShader;
  StackAllocator m_metadataAllocator;
};
} // namespace dx12
} // namespace SirEngine
