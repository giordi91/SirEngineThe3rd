#pragma once
#include "SirEngine/handle.h"

namespace SirEngine {
class RootSignatureManager;
class ShaderManager;
class ShadersLayoutRegistry;

class PSOManager {

public:
  PSOManager() = default;
  virtual ~PSOManager() = default;
  PSOManager(const PSOManager &) = delete;
  PSOManager &operator=(const PSOManager &) = delete;
  PSOManager(PSOManager &&) = delete;
  PSOManager &operator=(PSOManager &&) = delete;

  virtual void initialize() = 0;
  virtual void cleanup() = 0;
  virtual void loadRawPSOInFolder(const char *directory) = 0;
  virtual void loadCachedPSOInFolder(const char *directory) = 0;

  virtual void recompilePSOFromShader(const char *shaderName,
                                      const char *getOffsetPath) = 0;

  virtual PSOHandle getHandleFromName(const char *name) const = 0;
};
} // namespace SirEngine
