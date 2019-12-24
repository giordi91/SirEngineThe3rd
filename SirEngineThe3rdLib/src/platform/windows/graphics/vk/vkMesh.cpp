#include "platform/windows/graphics/vk/vkMesh.h"

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "meshoptimizer.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

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
          obj->positions[gi.p * 3 + 0],
          obj->positions[gi.p * 3 + 1],
          obj->positions[gi.p * 3 + 2],
          uint8_t(nx * 127.0f + 127.0f),
          uint8_t(ny * 127.0f + 127.0f),
          uint8_t(nz * 127.0f + 127.0f),
          0,
          obj->texcoords[gi.t * 2 + 0],
          obj->texcoords[gi.t * 2 + 1],
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

struct Normal {
  uint8_t nx, ny, nz, wz;
};
bool loadMeshDeInterleaved(const char *path, VkMesh &outMesh) {
  fastObjMesh *obj = fast_obj_read(path);
  if (!obj) {
    printf("Error loading %s: file not found\n", path);
    return false;
  }

  size_t totalIndices = 0;

  for (unsigned int i = 0; i < obj->face_count; ++i)
    totalIndices += 3 * (obj->face_vertices[i] - 2);

  std::vector<glm::vec3> positions(totalIndices);
  std::vector<Normal> normals(totalIndices);
  std::vector<glm::vec2> uvs(totalIndices);

  size_t vertexOffset = 0;
  size_t indexOffset = 0;

  for (unsigned int i = 0; i < obj->face_count; ++i) {
    for (unsigned int j = 0; j < obj->face_vertices[i]; ++j) {
      fastObjIndex gi = obj->indices[indexOffset + j];

      float nx = obj->normals[gi.n * 3 + 0];
      float ny = obj->normals[gi.n * 3 + 1];
      float nz = obj->normals[gi.n * 3 + 2];

      glm::vec3 p = {obj->positions[gi.p * 3 + 0], obj->positions[gi.p * 3 + 1],
                     obj->positions[gi.p * 3 + 2]};
      Normal n{uint8_t(nx * 127.0f + 127.0f), uint8_t(ny * 127.0f + 127.0f),
               uint8_t(nz * 127.0f + 127.0f), 0};
      glm::vec2 uv{obj->texcoords[gi.t * 2 + 0], obj->texcoords[gi.t * 2 + 1]};

      Vertex v = {
          obj->positions[gi.p * 3 + 0],
          obj->positions[gi.p * 3 + 1],
          obj->positions[gi.p * 3 + 2],
          uint8_t(nx * 127.0f + 127.0f),
          uint8_t(ny * 127.0f + 127.0f),
          uint8_t(nz * 127.0f + 127.0f),
          0,
          obj->texcoords[gi.t * 2 + 0],
          obj->texcoords[gi.t * 2 + 1],
      };

      // triangulate polygon on the fly; offset-3 is always the first polygon
      // vertex
      if (j >= 3) {
        positions[vertexOffset + 0] = positions[vertexOffset - 3];
        positions[vertexOffset + 1] = positions[vertexOffset - 1];

        normals[vertexOffset + 0] = normals[vertexOffset - 3];
        normals[vertexOffset + 1] = normals[vertexOffset - 1];

        uvs[vertexOffset + 0] = uvs[vertexOffset - 3];
        uvs[vertexOffset + 1] = uvs[vertexOffset - 1];

        vertexOffset += 2;
      }

      // vertices[vertexOffset] = v;
      positions[vertexOffset] = p;
      normals[vertexOffset] = n;
      uvs[vertexOffset] = uv;
      vertexOffset++;
    }

    indexOffset += obj->face_vertices[i];
  }

  fast_obj_destroy(obj);

  std::vector<unsigned int> remap(totalIndices);

  meshopt_Stream streams[] = {
      {&positions[0], sizeof(float) * 3, sizeof(float) * 3},
      {&normals[0], sizeof(float), sizeof(float)},
      {&uvs[0], sizeof(float) * 2, sizeof(float) * 2},
  };

  const size_t totalVertices = meshopt_generateVertexRemapMulti(
      &remap[0], NULL, totalIndices, totalIndices, streams, 3);

  outMesh.indices.resize(totalIndices);
  meshopt_remapIndexBuffer(&outMesh.indices[0], NULL, totalIndices, &remap[0]);

  outMesh.vertices.resize(totalVertices);
  auto *normalsOffset = reinterpret_cast<glm::vec3 *>(&outMesh.vertices[0]);
  normalsOffset += totalVertices;

  meshopt_remapVertexBuffer(&outMesh.vertices[0], &positions[0], totalIndices,
                            sizeof(glm::vec3), &remap[0]);
  meshopt_remapVertexBuffer(normalsOffset, &normals[0], totalIndices,
                            sizeof(Normal), &remap[0]);
  auto *uvOffset = reinterpret_cast<Normal *>(normalsOffset);
  uvOffset += totalVertices;
  meshopt_remapVertexBuffer(uvOffset, &uvs[0], totalIndices, sizeof(glm::vec2),
                            &remap[0]);

  return true;
}
} // namespace SirEngine::vk
