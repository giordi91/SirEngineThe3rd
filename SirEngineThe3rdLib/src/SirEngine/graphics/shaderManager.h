#pragma once

#include "SirEngine/memory/cpu/resizableVector.h"

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

  virtual void loadShaderBinaryFile(const char *path) = 0;
  virtual void loadShadersInFolder(const char *directory) = 0;

  virtual const char *recompileShader(const char *path, const char *offsetPath,
                                      bool &result) = 0;
  virtual const ResizableVector<const char *> &getShaderNames() = 0;
};
}  // namespace SirEngine::graphics
