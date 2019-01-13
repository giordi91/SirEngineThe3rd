#pragma once
#include "vendor/tiny_obj_loader.h"
#include <DirectXMath.h>
#include <memory>

struct Model {
  std::vector<float> vertices;
  std::vector<int> indices;
  int strideInByte;
  int vertexCount;
  int triangleCount;
};

void convertObj(const tinyobj::attrib_t &attr, const tinyobj::shape_t &shape,
                Model &model, const std::string &tangentsPath,
                const std::string &skinPath);
