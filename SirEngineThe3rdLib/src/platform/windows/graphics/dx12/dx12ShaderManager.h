#pragma once
#include <d3dcommon.h>

#include <cassert>

#include "SirEngine/graphics/shaderManager.h"
#include "SirEngine/memory/resizableVector.h"
#include "SirEngine/memory/stackAllocator.h"
#include "SirEngine/memory/stringHashMap.h"

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
  // right now this is empty, is kept here for the time being
  // just for symmetry with the other managers
  void initialize() override;
  void loadShadersInFolder(const char *directory) override;
  void cleanup() override;
  inline ID3DBlob *getShaderFromName(const std::string &name) const {
    /*const auto found = m_stringToShader.find(name);
    if (found != m_stringToShader.end()) {
      return found->second.shader;
    }
    */
    // TODO ideally here we would want to get a ref, we will need to expand the
    // map to do so
    ShaderBlob blob;
    bool result = m_stringToShader.get(name.c_str(), blob);
    if (result) {
      return blob.shader;
    }
    if (name == "null") {
      return nullptr;
    }
    assert(0 && "could not find shader");
    return nullptr;
  }

  const ResizableVector<const char *> &getShaderNames() override {
    return m_shaderNames;
  }

  Dx12ShaderManager() : m_stringToShader(400), m_shaderNames(400) {}
  ~Dx12ShaderManager();
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
  HashMap<const char *, ShaderBlob, hashString32> m_stringToShader;
  ResizableVector<const char *> m_shaderNames;
  StackAllocator m_metadataAllocator;
  DXCShaderCompiler *m_compiler = nullptr;
};
}  // namespace SirEngine::dx12
