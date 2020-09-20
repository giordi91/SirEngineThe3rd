#pragma once
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "handle.h"

namespace SirEngine {

class MeshManager {
 public:
  virtual ~MeshManager() = default;
  MeshManager() = default;
  MeshManager(const MeshManager &) = delete;
  MeshManager &operator=(const MeshManager &) = delete;
  virtual void initialize() = 0;
  virtual void cleanup() = 0;

  virtual MeshHandle loadMesh(const char *path) = 0;
  virtual MeshHandle getHandleFromName(const char *name) const = 0;

  virtual void free(const MeshHandle handle) = 0;
  virtual const BoundingBox *getBoundingBoxes(uint32_t &outSize) const = 0;
};

}  // namespace SirEngine