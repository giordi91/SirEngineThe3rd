#pragma once
#include <vector>

namespace SirEngine::vk {

struct AttributeRange {
  uint64_t size;
  uint64_t offset;
};

struct VkMesh {
  std::vector<float> m_vertices;
  std::vector<uint32_t> m_indices;
  uint32_t m_vertexCount;
  AttributeRange m_positions;
  AttributeRange m_normals;
  AttributeRange m_uv;
};

bool loadMeshDeInterleaved(const char *path, VkMesh &outMesh);

} // namespace SirEngine::vk