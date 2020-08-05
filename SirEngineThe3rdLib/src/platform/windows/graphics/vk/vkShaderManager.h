#pragma once
#include <cassert>
#include <string>
#include <unordered_map>

#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/graphics/shaderManager.h"
#include "SirEngine/memory/cpu/stackAllocator.h"
#include "SirEngine/memory/cpu/stringHashMap.h"
#include "platform/windows/graphics/vk/volk.h"

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

class VkShaderManager : public graphics::ShaderManager {
 public:
  VkShaderManager() : m_stringToShader(400), m_shaderNames(400) {}
  ~VkShaderManager();
  // right now this is empty, is kept here for the time being
  // just for symmetry with the other managers
  void initialize() override;
  void loadShaderFile(const char *path) override;
  void loadShadersInFolder(const char *directory) override;
  void cleanup() override;

  VkShaderModule getShaderFromName(const char *name) const {
    // TODO ideally here we would want to get a ref, we will need to expand the
    // hash map to do so
    VkShaderBlob blob;
    bool result = m_stringToShader.get(name, blob);
    if (result) {
      return blob.shader;
    }
    if (strcmp(name, "null") == 0) {
      return nullptr;
    }
    assert(0 && "could not find shader");
    return nullptr;
  }
  const ResizableVector<const char *> &getShaderNames() override {
    return m_shaderNames;
  }

  VkShaderManager(const VkShaderManager &) = delete;
  VkShaderManager &operator=(const VkShaderManager &) = delete;
  void loadShaderBinaryFile(const char *path) override;
  const char *recompileShader(const char *path, const char *offsetPath,
                              bool &result) override;

 private:
  // 2 mb of data for the stack
  const int METADATA_STACK_SIZE = 1 << 21;
  // data caching
  HashMap<const char *, VkShaderBlob, hashString32> m_stringToShader;
  ResizableVector<const char *> m_shaderNames;
  StackAllocator m_metadataAllocator;
  VkShaderCompiler *m_compiler = nullptr;
};
}  // namespace SirEngine::vk
