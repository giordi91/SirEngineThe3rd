#pragma once
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "handle.h"

namespace SirEngine {

enum MeshAttributeFlags
{
    POSITIONS =1,
    NORMALS = 2,
	UV = 4,
	TANGENTS=8,
	ALL=15
};

class MeshManager {

public:
	virtual ~MeshManager() = default;
	MeshManager() = default;
  MeshManager(const MeshManager &) = delete;
  MeshManager &operator=(const MeshManager &) = delete;


  // TODO fix is internal
  virtual MeshHandle loadMesh(const char *path, bool isInternal = false)=0;

  virtual void free(const MeshHandle handle) = 0;
  virtual const BoundingBox * getBoundingBoxes(uint32_t &outSize) const = 0;
};

} // namespace SirEngine