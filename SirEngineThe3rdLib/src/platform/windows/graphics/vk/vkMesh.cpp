#include "platform/windows/graphics/vk/vkMesh.h"

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "meshoptimizer.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace SirEngine::vk {

struct Normal {
  uint8_t nx, ny, nz, wz;
};

uint64_t alignSize(const uint64_t sizeInBytes, const uint64_t boundaryInByte,
                   uint64_t &offset) {
  uint64_t modulus = sizeInBytes % boundaryInByte;
  offset = modulus;
  return sizeInBytes + modulus;
}

bool loadMeshDeInterleaved(const char *path, VkMesh &outMesh) {
  fastObjMesh *obj = fast_obj_read(path);
  if (!obj) {
    printf("Error loading %s: file not found\n", path);
    return false;
  }

  size_t totalIndices = 0;

  for (unsigned int i = 0; i < obj->face_count; ++i)
    totalIndices += 3 * (obj->face_vertices[i] - 2);

  std::vector<glm::vec4> positions(totalIndices);
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

      glm::vec4 p = {obj->positions[gi.p * 3 + 0], obj->positions[gi.p * 3 + 1],
                     obj->positions[gi.p * 3 + 2], 1.0f};
      Normal n{uint8_t(nx * 127.0f + 127.0f), uint8_t(ny * 127.0f + 127.0f),
               uint8_t(nz * 127.0f + 127.0f), 0};
      glm::vec2 uv{obj->texcoords[gi.t * 2 + 0], obj->texcoords[gi.t * 2 + 1]};

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
      {&positions[0], sizeof(float) * 4, sizeof(float) * 4},
      {&normals[0], sizeof(float), sizeof(float)},
      {&uvs[0], sizeof(float) * 2, sizeof(float) * 2},
  };

  const size_t totalVertices = meshopt_generateVertexRemapMulti(
      &remap[0], NULL, totalIndices, totalIndices, streams, 3);
  outMesh.m_vertexCount = totalVertices;

  // compute offsets and alignment
  outMesh.m_positions.size = totalVertices * 4 * sizeof(float);
  outMesh.m_positions.offset = 0;

  uint64_t pointsSize = outMesh.m_positions.size;
  uint64_t normalPointerOffset = 0;
  uint64_t normalsOffsetByte =
      alignSize(pointsSize, sizeof(float) * 4, normalPointerOffset);
  outMesh.m_normals.size = totalVertices * sizeof(float);
  outMesh.m_normals.offset = normalsOffsetByte;

  uint64_t normalsSize = outMesh.m_normals.size;
  uint64_t uvPointerOffset = 0;
  uint64_t uvOffsetByte =
      alignSize(normalsSize, sizeof(float) * 4, uvPointerOffset);
  outMesh.m_uv.size = totalVertices * sizeof(float) * 2;
  outMesh.m_uv.offset = uvOffsetByte + outMesh.m_normals.offset;

  outMesh.m_indices.resize(totalIndices);
  meshopt_remapIndexBuffer(&outMesh.m_indices[0], NULL, totalIndices,
                           &remap[0]);

  // total size is 4 floats for positions 1 float for normals and two floats for
  // uv plus the aligment offset needed for normals and uv
  outMesh.m_vertices.resize(totalVertices * (4 + 1 + 2) +
                            (normalPointerOffset + uvPointerOffset) /
                                sizeof(float));

  char *startPointer = reinterpret_cast<char *>(&outMesh.m_vertices[0]);
  char *normalsOffset = startPointer + outMesh.m_normals.offset;
  auto *uvOffset = startPointer + outMesh.m_uv.offset;

  meshopt_remapVertexBuffer(&outMesh.m_vertices[0], &positions[0], totalIndices,
                            sizeof(glm::vec4), &remap[0]);
  meshopt_remapVertexBuffer(normalsOffset, &normals[0], totalIndices,
                            sizeof(Normal), &remap[0]);
  meshopt_remapVertexBuffer(uvOffset, &uvs[0], totalIndices, sizeof(glm::vec2),
                            &remap[0]);

  return true;
}
} // namespace SirEngine::vk
