#pragma once
#include "tinyobjloader/tiny_obj_loader.h"

struct Model {
  std::vector<float> vertices;
  std::vector<uint32_t> indices;
  int strideInByte;
  int vertexCount;
  int triangleCount;
  float boundingBox[6];
};

struct Vertex {
  float vx, vy, vz;
  float nx, ny, nz;
  float u, v;
  float tx,ty,tz,tw;
};

struct SkinData {
  std::vector<int> jnts;
  std::vector<float> weights;
};

void convertObj(const tinyobj::attrib_t &attr, const tinyobj::shape_t &shape,
                Model &model, SkinData &finalSkinData,
                const std::string &tangentsPath, const std::string &skinPath);

bool convertObj(const char *path, const char *tangentsPath,
                const char *skinPath, SkinData& finalSkinData, Model &model);
