#pragma once
#include "SirEngine/handle.h"

namespace SirEngine {

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
  virtual void bindPSO(PSOHandle handle) const = 0;
public:
	static const uint32_t PER_FRAME_DATA_BINDING_INDEX = 0;
	static const uint32_t STATIC_SAMPLERS_BINDING_INDEX = 1;
	static const uint32_t PER_PASS_BINDING_INDEX = 2;
	static const uint32_t PER_OBJECT_BINDING_INDEX = 3;
};
} // namespace SirEngine
