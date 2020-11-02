#pragma once
#include "SirEngine/graphics/graphicsDefines.h"
#include "tinyobjloader/tiny_obj_loader.h"

struct Model {
  std::vector<float> vertices;
  std::vector<uint32_t> indices;
  uint32_t vertexCount;
  uint32_t indexCount;
  uint32_t triangleCount;
  float boundingBox[6];
  SirEngine::MemoryRange positionRange;
  SirEngine::MemoryRange normalsRange;
  SirEngine::MemoryRange uvRange;
  SirEngine::MemoryRange tangentsRange;
  uint32_t flags;
};

struct Vertex {
  float vx, vy, vz;
  float nx, ny, nz;
  float u, v;
  float tx, ty, tz, tw;
};

struct SkinData {
  std::vector<int> jnts;
  std::vector<float> weights;
};

void convertObj(const tinyobj::attrib_t &attr, const tinyobj::shape_t &shape,
                Model &model, SkinData &finalSkinData,
                const std::string &tangentsPath, const std::string &skinPath);

bool convertObj(const char *path, const char *tangentsPath,
                const char *skinPath, SkinData &finalSkinData, Model &model);
