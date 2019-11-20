#include "platform/windows/graphics/vk/vkMesh.h"

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "meshoptimizer.h"
#include <cassert>

namespace SirEngine::vk {
bool loadMesh(const char *path, VkMesh &outMesh) {
  fastObjMesh *obj = fast_obj_read(path);
  if (!obj) {
    printf("Error loading %s: file not found\n", path);
    return false;
  }

  size_t totalIndices = 0;

  for (unsigned int i = 0; i < obj->face_count; ++i)
    totalIndices += 3 * (obj->face_vertices[i] - 2);

  std::vector<Vertex> vertices(totalIndices);

  size_t vertexOffset = 0;
  size_t indexOffset = 0;

  for (unsigned int i = 0; i < obj->face_count; ++i) {
    for (unsigned int j = 0; j < obj->face_vertices[i]; ++j) {
      fastObjIndex gi = obj->indices[indexOffset + j];

      float nx = obj->normals[gi.n * 3 + 0];
      float ny = obj->normals[gi.n * 3 + 1];
      float nz = obj->normals[gi.n * 3 + 2];

      Vertex v = {
          obj->positions[gi.p * 3 + 0], obj->positions[gi.p * 3 + 1],
          obj->positions[gi.p * 3 + 2], uint8_t(nx*127.0f + 127.0f),
          uint8_t(ny*127.0f + 127.0f),   uint8_t(nz*127.0f + 127.0f),
		  0.0f,
          obj->texcoords[gi.t * 2 + 0], obj->texcoords[gi.t * 2 + 1],
      };

      // triangulate polygon on the fly; offset-3 is always the first polygon
      // vertex
      if (j >= 3) {
        vertices[vertexOffset + 0] = vertices[vertexOffset - 3];
        vertices[vertexOffset + 1] = vertices[vertexOffset - 1];
        vertexOffset += 2;
      }

      vertices[vertexOffset] = v;
      vertexOffset++;
    }

    indexOffset += obj->face_vertices[i];
  }

  /**/
  fast_obj_destroy(obj);

  std::vector<unsigned int> remap(totalIndices);

  const size_t totalVertices =
      meshopt_generateVertexRemap(&remap[0], NULL, totalIndices, &vertices[0],
                                  totalIndices, sizeof(Vertex));

  outMesh.indices.resize(totalIndices);
  meshopt_remapIndexBuffer(&outMesh.indices[0], NULL, totalIndices, &remap[0]);

  outMesh.vertices.resize(totalVertices);
  meshopt_remapVertexBuffer(&outMesh.vertices[0], &vertices[0], totalIndices,
                            sizeof(Vertex), &remap[0]);
  return true;
}
} // namespace vk
