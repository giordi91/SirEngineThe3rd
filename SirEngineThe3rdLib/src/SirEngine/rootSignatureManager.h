#pragma once

#include "SirEngine/handle.h"

namespace SirEngine {

enum class ROOT_FILE_TYPE { RASTER = 0, COMPUTE = 1, DXR = 2, NULL_TYPE };

class RootSignatureManager {

public:
  RootSignatureManager() = default;
  RootSignatureManager(const RootSignatureManager &) = delete;
  RootSignatureManager &operator=(const RootSignatureManager &) = delete;
  RootSignatureManager(RootSignatureManager &&) = delete;
  RootSignatureManager &operator=(RootSignatureManager &&) = delete;

  virtual ~RootSignatureManager() = default;
  virtual void initialize() =0;
  virtual void cleanup() = 0;
  virtual void loadSignaturesInFolder(const char *directory) = 0;
  virtual void loadSignatureBinaryFile(const char *file) = 0;

  virtual RSHandle getHandleFromName(const char *name) const = 0;
};
} // namespace SirEngine
