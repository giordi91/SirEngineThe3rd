#define TINYOBJLOADER_IMPLEMENTATION

#include "processObj.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include <DirectXMath.h>
#include <limits>
#include <unordered_map>
#include <vector>

static const float VERTEX_DELTA = 0.00001f;
struct VertexCompare {
  DirectX::XMFLOAT3 p{};
  DirectX::XMFLOAT3 n{};
  DirectX::XMFLOAT2 uv{};
  DirectX::XMFLOAT3 t{};
  float pad5 = 0.0f;
};

inline int float2Compare(const DirectX::XMFLOAT2 a, const DirectX::XMFLOAT2 b) {
  //TODO fix fabs, for faster solution, if is an up/down cast to double
  bool xSame = std::fabsf(a.x - b.x) < VERTEX_DELTA;
  bool ySame = std::fabsf(a.y - b.y) < VERTEX_DELTA;
  return (xSame & ySame) ? 1 : 0;
}

inline int float3Compare(const DirectX::XMFLOAT3 a, const DirectX::XMFLOAT3 b) {
  //TODO fix fabs, for faster solution, if is an up/down cast to double
  bool xSame = std::fabsf(a.x - b.x) < VERTEX_DELTA;
  bool ySame = std::fabsf(a.y - b.y) < VERTEX_DELTA;
  bool zSame = std::fabsf(a.z - b.z) < VERTEX_DELTA;
  return (xSame & ySame & zSame) ? 1 : 0;
}

inline bool compareVertex(const VertexCompare &a, const VertexCompare &b) {
  int same = 0;
  same += float3Compare(a.p, b.p);
  same += float3Compare(a.n, b.n);
  same += float3Compare(a.t, b.t);
  same += float2Compare(a.uv, b.uv);
  return same == 4;
}
bool operator==(const VertexCompare &lhs, const VertexCompare &rhs) {
  return compareVertex(lhs, rhs);
}

const int primes[] = {73856093, 19349663, 83492791};

namespace std {
template <> struct hash<DirectX::XMFLOAT2> {
  size_t operator()(DirectX::XMFLOAT2 const &v) const noexcept {
    auto h1 = std::hash<float>{}(v.x);
    auto h2 = std::hash<float>{}(v.y);

    return h1 ^ (h2 << 1);
  }
};

template <> struct hash<DirectX::XMFLOAT3>final
{
  size_t operator()(DirectX::XMFLOAT3 const &v) const noexcept {
    auto h1 = std::hash<float>{}(v.x);
    auto h2 = std::hash<float>{}(v.y);
    auto h3 = std::hash<float>{}(v.z);

    return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
  }
};
} // namespace std

struct HashFunc final
{
  size_t operator()(const VertexCompare &k) const {
    return std::hash<DirectX::XMFLOAT3>()(k.p) ^
           (std::hash<DirectX::XMFLOAT3>()(k.n) << 1) ^
           (std::hash<DirectX::XMFLOAT3>()(k.t) << 2) ^
           (std::hash<DirectX::XMFLOAT2>()(k.uv) << 3);
  }
};

struct EqualsFunc final
{
  bool operator()(const VertexCompare &lhs, const VertexCompare &rhs) const {
    return lhs == rhs;
  }
};

std::vector<float> loadTangents(const std::string& tanPath) {
  assert(fileExists(tanPath));
  nlohmann::json jObj = getJsonObj(tanPath);
  size_t sz = jObj.size();
  std::vector<float> tempT;
  tempT.resize(sz);
  size_t counter = 0;
  for (const auto& t : jObj) {
    tempT[counter] = t.get<float>();
    assert(counter < sz);
    ++counter;
  }
  return tempT;
}

void convertObjNoTangents(const tinyobj::attrib_t &attr,
                          const tinyobj::shape_t &shape, Model &model,
                          const std::string &skinPath) {

  // how it works:
  // first of all we need  to iterate over all the geometry, we expand all
  // the auxiliary indices, meaning uv,normals etc indices, and we expand
  // that into a vertex structure that we track with the vertex buffer
  // index. every vertex we add it to a struct, so we know how many faces
  // use the vertex, or how many faces share a vertex.

  std::unordered_map<VertexCompare, uint32_t, HashFunc, EqualsFunc>
      uniqueVertices = {};
  std::vector<int> indices;
  std::vector<VertexCompare> vertexData;

  // Loop over faces(polygon)
  size_t indexOffset = 0;
  int indexCount = 0;
  for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
    size_t fv = shape.mesh.num_face_vertices[f];
    assert(fv == 3);

    // Loop over vertices in the face.
    for (size_t v = 0; v < fv; v++) {
      // access to vertex
      tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
      VertexCompare c;
      c.p.x = attr.vertices[3 * idx.vertex_index + 0];
      c.p.y = attr.vertices[3 * idx.vertex_index + 1];
      c.p.z = attr.vertices[3 * idx.vertex_index + 2];
      c.n.x = attr.normals[3 * idx.normal_index + 0];
      c.n.y = attr.normals[3 * idx.normal_index + 1];
      c.n.z = attr.normals[3 * idx.normal_index + 2];
      c.uv.x = attr.texcoords[2 * idx.texcoord_index + 0];
      c.uv.y = attr.texcoords[2 * idx.texcoord_index + 1];
      c.t.x = 0.0f;
      c.t.y = 0.0f;
      c.t.z = 0.0f;

      // if the vertex is not in the map, it means is unique
      // and needs to be added and is a valid vertex in the vertex buffer
      if (uniqueVertices.count(c) == 0) {
        uniqueVertices[c] = indexCount++;
        vertexData.push_back(c);
      }

      indices.push_back(uniqueVertices[c]);
    }
    indexOffset += fv;
  }

  // model is loaded, lets copy data to output struct
  size_t indicesCount = indices.size();
  size_t vertexCompareCount = vertexData.size();
  uint32_t stride = 12;

  model.indices.resize(indicesCount);
  model.vertices.resize(vertexCompareCount * stride);
  model.vertexCount = static_cast<int>(vertexCompareCount);
  model.strideInByte = sizeof(float) * stride;
  model.triangleCount = static_cast<int>(shape.mesh.num_face_vertices.size());

  memcpy(model.vertices.data(), vertexData.data(),
         vertexCompareCount * stride * sizeof(float));
  memcpy(model.indices.data(), indices.data(), indicesCount * sizeof(float));
}

void convertObj(const tinyobj::attrib_t &attr, const tinyobj::shape_t &shape,
                Model &model, const std::string &tangentsPath,
                const std::string &skinPath) {

  if (tangentsPath.empty()) {
    convertObjNoTangents(attr, shape, model, skinPath);
    return;
  }

  // here we need to process a model which has tangents
  std::vector<float> tangents = loadTangents(tangentsPath);

  // how it works:
  // first of all we need  to iterate over all the geometry, we expand all
  // the auxiliary indices, meaning uv,normals etc indices, and we expand
  // that into a vertex structure that we track with the vertex buffer
  // index. every vertex we add it to a struct, so we know how many faces
  // use the vertex, or how many faces share a vertex.

  std::unordered_map<VertexCompare, uint32_t, HashFunc, EqualsFunc>
      uniqueVertices = {};
  std::vector<int> indices;
  std::vector<VertexCompare> vertexData;

  // Loop over faces(polygon)
  size_t index_offset = 0;
  int indexCount = 0;
  for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
    size_t fv = shape.mesh.num_face_vertices[f];
    assert(fv == 3);

    // Loop over vertices in the face.
    for (size_t v = 0; v < fv; v++) {
      // access to vertex
      tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
      VertexCompare c;
      c.p.x = attr.vertices[3 * idx.vertex_index + 0];
      c.p.y = attr.vertices[3 * idx.vertex_index + 1];
      c.p.z = attr.vertices[3 * idx.vertex_index + 2];
      c.n.x = attr.normals[3 * idx.normal_index + 0];
      c.n.y = attr.normals[3 * idx.normal_index + 1];
      c.n.z = attr.normals[3 * idx.normal_index + 2];
      c.uv.x = attr.texcoords[2 * idx.texcoord_index + 0];
      c.uv.y = attr.texcoords[2 * idx.texcoord_index + 1];
      c.t.x = tangents[3 * idx.vertex_index + 0];
      c.t.y = tangents[3 * idx.vertex_index + 1];
      c.t.z = tangents[3 * idx.vertex_index + 2];

      // if the vertex is not in the map, it means is unique
      // and needs to be added and is a valid vertex in the vertex buffer
      if (uniqueVertices.count(c) == 0) {
        uniqueVertices[c] = indexCount++;
        vertexData.push_back(c);
      }

      indices.push_back(uniqueVertices[c]);
    }
    index_offset += fv;

    // per-face material
    // shapes[s].mesh.material_ids[f];
  }

  // model is loaded, lets copy data to output struct
  size_t indicesCount = indices.size();
  size_t vertexCompareCount = vertexData.size();
  uint32_t stride = 12;

  model.indices.resize(indicesCount);
  model.vertices.resize(vertexCompareCount * stride);
  model.vertexCount = static_cast<int>(vertexCompareCount);
  model.strideInByte = sizeof(float) * stride;
  model.triangleCount = static_cast<int>(shape.mesh.num_face_vertices.size());

  memcpy(model.vertices.data(), vertexData.data(),
         vertexCompareCount * stride * sizeof(float));
  memcpy(model.indices.data(), indices.data(), indicesCount * sizeof(float));
}
