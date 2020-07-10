#pragma once

#include "SirEngine/memory/resizableVector.h"

namespace SirEngine::graphics {

class ShaderManager {
 public:
  virtual ~ShaderManager() = default;
  ShaderManager() = default;
  ShaderManager(const ShaderManager &) = delete;
  ShaderManager &operator=(const ShaderManager &) = delete;
  ShaderManager(ShaderManager &&o) noexcept = delete;        // move constructor
  ShaderManager &operator=(ShaderManager &&other) = delete;  // move assignment

  // right now this is empty, is kept here for the time being
  // just for symmetry with the other managers
  virtual void initialize() = 0;
  virtual void cleanup() = 0;

  virtual void loadShaderFile(const char *path) = 0;
  virtual void loadShaderBinaryFile(const char *path) = 0;
  virtual void loadShadersInFolder(const char *directory) = 0;

  // TODO ideally I would like to remove the string from interface
  // the log is used to show in the ui the result of compilation
  // we can probably return a const char* and let the user combine it
  virtual const char *recompileShader(const char *path,
                                      const char *offsetPath) = 0;
  virtual const ResizableVector<const char *> &getShaderNames() = 0;
};
}  // namespace SirEngine::graphics
