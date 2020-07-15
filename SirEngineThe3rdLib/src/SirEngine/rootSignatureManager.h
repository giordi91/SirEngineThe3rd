#pragma once

#include "SirEngine/handle.h"

namespace SirEngine {

enum class ROOT_FILE_TYPE { NULL_TYPE, RASTER = 1, COMPUTE = 2, DXR = 3 };

struct RootDefinition
{
	ROOT_FILE_TYPE rootType;

	
};

struct RootComponentDefinition
{
	
};

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
  //virtual RSHandle generateRootSignatureFromDescriptrion() = 0;
};
} // namespace SirEngine
