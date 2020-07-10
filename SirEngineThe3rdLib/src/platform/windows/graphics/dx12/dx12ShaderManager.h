#pragma once
#include <d3dcommon.h>

#include <cassert>
#include <string>
#include <unordered_map>

#include "SirEngine/graphics/shaderManager.h"
#include "SirEngine/memory/stackAllocator.h"

namespace SirEngine::dx12 {
struct ShaderArgs;

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

class Dx12ShaderManager final : public graphics::ShaderManager {
 public:
  ~Dx12ShaderManager();
  // right now this is empty, is kept here for the time being
  // just for symmetry with the other managers
  void initialize() override;
  void loadShadersInFolder(const char *directory) override;
  void cleanup() override;
  inline ID3DBlob *getShaderFromName(const std::string &name) {
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

  const std::vector<std::string> &getShaderNames() override {
    return m_shaderNames;
  }

  Dx12ShaderManager() = default;
  Dx12ShaderManager(const Dx12ShaderManager &) = delete;
  Dx12ShaderManager &operator=(const Dx12ShaderManager &) = delete;
  void loadShaderFile(const char *path) override;
  void loadShaderBinaryFile(const char *path) override;
  void recompileShader(const char *path, const char *offsetPath,
                       std::string *log) override;

 private:
  // 2 mb of data for the stack
  const int METADATA_STACK_SIZE = 1 << 21;
  // data caching
  std::unordered_map<std::string, ShaderBlob> m_stringToShader;
  std::vector<std::string> m_shaderNames;
  StackAllocator m_metadataAllocator;
  DXCShaderCompiler *m_compiler;
};
}  // namespace SirEngine::dx12
