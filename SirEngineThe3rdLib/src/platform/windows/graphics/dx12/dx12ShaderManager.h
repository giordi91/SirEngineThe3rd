#pragma once

#include <cassert>

#include "SirEngine/graphics/shaderManager.h"
#include "SirEngine/memory/resizableVector.h"
#include "SirEngine/memory/stackAllocator.h"
#include "SirEngine/memory/stringHashMap.h"

struct ID3D10Blob;
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
  ID3D10Blob *shader;
  ShaderMetadata *metadata;
};

class Dx12ShaderManager final : public graphics::ShaderManager {
 public:
  // right now this is empty, is kept here for the time being
  // just for symmetry with the other managers
  void initialize() override;
  void loadShadersInFolder(const char *directory) override;
  void cleanup() override;
  inline ID3D10Blob *getShaderFromName(const char *name) const {
    // TODO ideally here we would want to get a ref, we will need to expand the
    // hash map to do so
    ShaderBlob blob;
    bool result = m_stringToShader.get(name, blob);
    if (result) {
      return blob.shader;
    }
    if (strcmp(name ,"null")==0) {
      return nullptr;
    }
    assert(0 && "could not find shader");
    return nullptr;
  }

  const ResizableVector<const char *> &getShaderNames() override {
    return m_shaderNames;
  }

  Dx12ShaderManager() : m_stringToShader(400), m_shaderNames(400) {}
  virtual ~Dx12ShaderManager();
  Dx12ShaderManager(const Dx12ShaderManager &) = delete;
  Dx12ShaderManager &operator=(const Dx12ShaderManager &) = delete;
  Dx12ShaderManager(Dx12ShaderManager &&o) noexcept =
      delete;  // move constructor
  Dx12ShaderManager &operator=(Dx12ShaderManager &&other) =
      delete;  // move assignment

  void loadShaderFile(const char *path) override;
  void loadShaderBinaryFile(const char *path) override;
  const char *recompileShader(const char *path,
                              const char *offsetPath) override;

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
