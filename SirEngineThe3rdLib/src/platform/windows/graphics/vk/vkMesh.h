#pragma once
#include <vector>

namespace SirEngine::vk {

struct Vertex {
  float vx, vy, vz;
  uint8_t nx, ny, nz,wz;
  float tu, tv;
};
struct VkMesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

bool loadMesh(const char *path, VkMesh &outMesh);

} // namespace vk