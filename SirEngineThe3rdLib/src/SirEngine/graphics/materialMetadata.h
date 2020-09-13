#pragma once
#include "graphicsDefines.h"

namespace SirEngine::graphics {
enum class MATERIAL_RESOURCE_TYPE { TEXTURE, CONSTANT_BUFFER, BUFFER };
enum class MATERIAL_RESOURCE_FLAGS {
  NONE = 0,
  READ_ONLY = 1,
  MESH_VERTICES = 2,
  MESH_NORMALS = 4,
  MESH_UVS = 8,
  MESH_TANGENTS = 16,
};

struct MaterialMeshBinding {
  int binding;
  MESH_ATTRIBUTE_FLAGS flags;
};
struct MaterialResource {
  MATERIAL_RESOURCE_TYPE type;
  GRAPHIC_RESOURCE_VISIBILITY visibility;
  const char *name;
  MATERIAL_RESOURCE_FLAGS flags;
  uint16_t set;
  uint16_t binding;
};
struct MaterialMetadata {
  MaterialResource *objectResources;
  MaterialResource *frameResources;
  MaterialResource *passResources;
  uint32_t objectResourceCount;
  uint32_t frameResourceCount;
  uint32_t passResourceCount;
  MaterialMeshBinding meshBinding;
};

MaterialMetadata SIR_ENGINE_API extractMetadata(const char *psoPath);
MaterialMetadata SIR_ENGINE_API loadMetadata(const char *psoPath,
                                             GRAPHIC_API api);
MaterialMetadata loadBinaryMetadata(const char* psoPath);

}  // namespace SirEngine::graphics
