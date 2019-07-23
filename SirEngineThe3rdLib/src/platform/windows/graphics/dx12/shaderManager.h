#pragma once
#include <d3dcommon.h>
#include <cassert>
#include <string>
#include <unordered_map>
#include "SirEngine/memory/stackAllocator.h"

namespace SirEngine {
namespace dx12 {

class DXCShaderCompiler;

struct ShaderMetadata {
  wchar_t *type;
  wchar_t *entryPoint;
  char *shaderPath;
  unsigned int shaderFlags;
  wchar_t *compilerArgs;
};
struct ShaderBlob {
  ID3DBlob *shader;
  ShaderMetadata *metadata;
};

class ShaderManager {
 public:
  ~ShaderManager();
  // right now this is empty, is kept here for the time being
  // just for simmetry with the other managers
  void init();
  void loadShadersInFolder(const char *directory);
  void cleanup();
  inline ID3DBlob *getShaderFromName(const std::string &name) {
    const auto found = m_stringToShader.find(name);
    if (found != m_stringToShader.end()) {
      return found->second.shader;
    }
    assert(0 && "could not find shader");
    return nullptr;
  }

  inline const std::unordered_map<std::string, ShaderBlob> &getShaderMap() {
    return m_stringToShader;
  }

  ShaderManager() = default;
  ShaderManager(const ShaderManager &) = delete;
  ShaderManager &operator=(const ShaderManager &) = delete;
  void loadShaderFile(const char *path);
  void loadShaderBinaryFile(const char *path);
  void recompileShader(const char *path, const char *offsetPath,
                       std::string *log);

 private:
  // 2 mb of data for the stack
  const int METADATA_STACK_SIZE = 1 << 21;
  // data caching
  std::unordered_map<std::string, ShaderBlob> m_stringToShader;
  StackAllocator m_metadataAllocator;
  DXCShaderCompiler *m_compiler;
};
}  // namespace dx12
}  // namespace SirEngine
