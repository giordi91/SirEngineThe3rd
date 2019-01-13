#include "processObj.h"
#include "vendor/tiny_obj_loader.h"
#include <DirectXMath.h>
#include <limits>
#include <unordered_map>
#include <vector>
#include "ResourceCompilerLib/resourceCompiler/fileUtils.h"
#include "ResourceCompilerLib/vendor/nlohmann/json.hpp"
#include <fstream>
#include <sstream>
#include "ResourceCompilerLib/resourceCompiler/log.h"

static const float VERTEX_DELTA = 0.00001f;
struct VertexCompare {
  DirectX::XMFLOAT3 p;
  float pad1 = 1.0f;
  DirectX::XMFLOAT3 n;
  float pad2 = 0.0f;
  DirectX::XMFLOAT2 uv;
  float pad3 = 0.0f;
  float pad4 = 0.0f;
  DirectX::XMFLOAT3 t;
  float pad5 = 0.0f;
};

inline int float2Compare(const DirectX::XMFLOAT2 a, const DirectX::XMFLOAT2 b) {
  // fix fabs, for faster solution
  bool xSame = std::fabsf(a.x - b.x) < VERTEX_DELTA;
  bool ySame = std::fabsf(a.y - b.y) < VERTEX_DELTA;
  return (xSame & ySame) ? 1 : 0;
}

inline int float3Compare(const DirectX::XMFLOAT3 a, const DirectX::XMFLOAT3 b) {
  // fix fabs, for faster solution
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

template <> struct hash<DirectX::XMFLOAT3> {
  size_t operator()(DirectX::XMFLOAT3 const &v) const noexcept {
    auto h1 = std::hash<float>{}(v.x);
    auto h2 = std::hash<float>{}(v.y);
    auto h3 = std::hash<float>{}(v.z);

    return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
  }
};
} // namespace std

struct hashFunc {
  size_t operator()(const VertexCompare &k) const {
    return std::hash<DirectX::XMFLOAT3>()(k.p) ^
           (std::hash<DirectX::XMFLOAT3>()(k.n) << 1) ^
           (std::hash<DirectX::XMFLOAT3>()(k.t) << 2) ^
           (std::hash<DirectX::XMFLOAT2>()(k.uv) << 3);
  }
};

struct equalsFunc {
  bool operator()(const VertexCompare &lhs, const VertexCompare &rhs) const {
    return lhs == rhs;
  }
};
inline nlohmann::json getJsonObj(std::string path) {

  bool res = fileExists(path);
  if (res) {
    // let s open the stream
    std::ifstream st(path);
    std::stringstream s_buffer;
    s_buffer << st.rdbuf();
    std::string s_buff = s_buffer.str();

    try {
      // try to parse
      nlohmann::json j_obj = nlohmann::json::parse(s_buff);
      return j_obj;
    } catch (...) {
      // if not lets throw an error
		SE_CORE_ERROR("Error parsing json file from path {0}", path);
      auto ex = std::current_exception();
      ex._RethrowException();
      return nlohmann::json();
    }
  } else {
    assert(0);
    return nlohmann::json();
  }
}

std::vector<float> loadTangents(std::string tan_path) {
  assert(fileExists(tan_path));
  nlohmann::json j_obj = getJsonObj(tan_path);
  int sz = j_obj.size();
  // int buffer_size = vtx_ids_size * 3;
  int buffer_size = sz;
  std::vector<float> temp_t;
  temp_t.resize(sz);
  int counter = 0;
  for (auto t : j_obj) {
    temp_t[counter] = t.get<float>();
    assert(counter < sz);
    ++counter;
  }
  return temp_t;
}

void convertObjNoTangents(const tinyobj::attrib_t &attr,
                          const tinyobj::shape_t &shape, Model &model,
                          const std::string &skinPath) {

  const float *const sourceVtx = attr.vertices.data();
  const float *const sourceNorm = attr.normals.data();
  const float *const sourceUv = attr.texcoords.data();

  // how it works:
  // first of all we need  to iterate over all the geometry, we expand all
  // the auxiliary indices, meaning uv,normals etc indices, and we expand
  // that into a vertex structure that we track with the vertex buffer
  // index. every vertex we add it to a struct, so we know how many faces
  // use the vertex, or how many faces share a vertex.

  std::unordered_map<VertexCompare, uint32_t, hashFunc, equalsFunc>
      uniqueVertices = {};
  std::vector<int> indices;
  std::vector<VertexCompare> vertexData;

  // Loop over faces(polygon)
  size_t index_offset = 0;
  int indexCount = 0;
  int maxInt = std::numeric_limits<int>::max();
  for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
    int fv = shape.mesh.num_face_vertices[f];
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
      c.t.x = 0.0f;
      c.t.y = 0.0f;
      c.t.z = 0.0f;

      assert(f < maxInt);

      // if the vertex is not in the map, it means is unique
      // and needs to be added and is a valid ne vertex in the vertex buffer
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
  int indicesCount = indices.size();
  int vertexCompareCount = vertexData.size();

  model.indices.resize(indicesCount);
  model.vertices.resize(vertexCompareCount * 16);
  model.vertexCount = vertexCompareCount;
  model.strideInByte = sizeof(float) * 16;
  model.triangleCount = shape.mesh.num_face_vertices.size();

  memcpy(model.vertices.data(), vertexData.data(),
         vertexCompareCount * 16 * sizeof(float));
  memcpy(model.indices.data(), indices.data(), indicesCount * sizeof(float));
}

void convertObj(const tinyobj::attrib_t &attr, const tinyobj::shape_t &shape,
                Model &model, const std::string &tangentsPath,
                const std::string &skinPath) {

  if (tangentsPath.empty()) {
    convertObjNoTangents(attr, shape, model, skinPath);
	return;
  }

  //here we need to process a model wich has tangents
  const float *const sourceVtx = attr.vertices.data();
  const float *const sourceNorm = attr.normals.data();
  const float *const sourceUv = attr.texcoords.data();

  std::vector<float> tangents = loadTangents(tangentsPath);

  // how it works:
  // first of all we need  to iterate over all the geometry, we expand all
  // the auxiliary indices, meaning uv,normals etc indices, and we expand
  // that into a vertex structure that we track with the vertex buffer
  // index. every vertex we add it to a struct, so we know how many faces
  // use the vertex, or how many faces share a vertex.

  std::unordered_map<VertexCompare, uint32_t, hashFunc, equalsFunc>
      uniqueVertices = {};
  std::vector<int> indices;
  std::vector<VertexCompare> vertexData;

  // Loop over faces(polygon)
  size_t index_offset = 0;
  int indexCount = 0;
  int maxInt = std::numeric_limits<int>::max();
  for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
    int fv = shape.mesh.num_face_vertices[f];
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

      assert(f < maxInt);

      // if the vertex is not in the map, it means is unique
      // and needs to be added and is a valid ne vertex in the vertex buffer
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
  int indicesCount = indices.size();
  int vertexCompareCount = vertexData.size();

  model.indices.resize(indicesCount);
  model.vertices.resize(vertexCompareCount * 16);
  model.vertexCount = vertexCompareCount;
  model.strideInByte = sizeof(float) * 16;
  model.triangleCount = shape.mesh.num_face_vertices.size();

  memcpy(model.vertices.data(), vertexData.data(),
         vertexCompareCount * 16 * sizeof(float));
  memcpy(model.indices.data(), indices.data(), indicesCount * sizeof(float));
}
