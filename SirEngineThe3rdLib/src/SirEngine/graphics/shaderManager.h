#pragma once

#include <string>
#include <unordered_map>

#include "SirEngine/handle.h"

namespace SirEngine::graphics {

class ShaderManager {
 public:
  virtual ~ShaderManager() = default;
  ShaderManager() = default;
  ShaderManager(const ShaderManager &) = delete;
  ShaderManager &operator=(const ShaderManager &) = delete;

  // right now this is empty, is kept here for the time being
  // just for symmetry with the other managers
  virtual void initialize() = 0;
  virtual void cleanup() = 0;

  virtual void loadShaderFile(const char *path) = 0;
  virtual void loadShaderBinaryFile(const char *path) = 0;
  virtual void loadShadersInFolder(const char *directory) = 0;

  virtual void recompileShader(const char *path, const char *offsetPath,
                               std::string *log) = 0;
  virtual const std::vector<std::string> &getShaderNames() = 0;
};
}  // namespace SirEngine::graphics
