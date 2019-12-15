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
  glm::vec3 p{};
  glm::vec3 n{};
  glm::vec2 uv{};
  glm::vec3 t{};
  float pad5 = 0.0f;
};

inline int float2Compare(const glm::vec2 a, const glm::vec2 b) {
  // TODO fix fabs, for faster solution, if is an up/down cast to double
  bool xSame = std::fabsf(a.x - b.x) < VERTEX_DELTA;
  bool ySame = std::fabsf(a.y - b.y) < VERTEX_DELTA;
  return (xSame & ySame) ? 1 : 0;
}

inline int float3Compare(const glm::vec3 a, const glm::vec3 b) {
  // TODO fix fabs, for faster solution, if is an up/down cast to double
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
template <> struct hash<glm::vec2> {
  size_t operator()(glm::vec2 const &v) const noexcept {
    auto h1 = std::hash<float>{}(v.x);
    auto h2 = std::hash<float>{}(v.y);

    return h1 ^ (h2 << 1);
  }
};

template <> struct hash<glm::vec3> final {
  size_t operator()(glm::vec3 const &v) const noexcept {
    auto h1 = std::hash<float>{}(v.x);
    auto h2 = std::hash<float>{}(v.y);
    auto h3 = std::hash<float>{}(v.z);

    return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
  }
};
} // namespace std

struct HashFunc final {
  size_t operator()(const VertexCompare &k) const {
    return std::hash<glm::vec3>()(k.p) ^
           (std::hash<glm::vec3>()(k.n) << 1) ^
           (std::hash<glm::vec3>()(k.t) << 2) ^
           (std::hash<glm::vec2>()(k.uv) << 3);
  }
};

struct EqualsFunc final {
  bool operator()(const VertexCompare &lhs, const VertexCompare &rhs) const {
    return lhs == rhs;
  }
};

std::vector<float> loadTangents(const std::string &tanPath) {
  assert(fileExists(tanPath));
  nlohmann::json jObj = getJsonObj(tanPath);
  size_t sz = jObj.size();
  std::vector<float> tempT;
  tempT.resize(sz);
  size_t counter = 0;
  for (const auto &t : jObj) {
    tempT[counter] = t.get<float>();
    assert(counter < sz);
    ++counter;
  }
  return tempT;
}

constexpr uint32_t VERTEX_INFLUENCE_COUNT = 6;
constexpr float SKIN_TOTAL_WEIGHT_TOLLERANCE = 0.001f;
bool loadSkin(const std::string &skinPath, SkinData &skinData) {
  if (skinPath.empty()) {
    return false;
  }
  // here we load the skincluster
  auto s_obj = getJsonObj(skinPath);
  // getting the skeleton name
  std::string sk = s_obj["skeleton"].get<std::string>();
  // allocating a new skincluster

  // allocating space for the joints and the weights
  auto data = s_obj["data"];
  auto size = s_obj["data"].size();
  //  auto jnts = static_cast<int *>(
  //      m_skin_alloc.allocate(size * VERTEX_INFLUENCE_COUNT * sizeof(int)));
  skinData.jnts.resize(size * VERTEX_INFLUENCE_COUNT);
  skinData.weights.resize(size * VERTEX_INFLUENCE_COUNT);
  // auto weights = static_cast<float *>(
  //    m_skin_alloc.allocate(size * VERTEX_INFLUENCE_COUNT * sizeof(float)));

  // those two variables are used to keep track
  // to where to write in the buffer
  int counter = 0;

  for (auto d : s_obj["data"]) {

    assert(d.find("j") != d.end());
    assert(d.find("w") != d.end());
    nlohmann::basic_json<> j = d["j"];
    nlohmann::basic_json<> w = d["w"];
    int id = counter * VERTEX_INFLUENCE_COUNT;

    // making sure we have the expected amount of data for each joint
    assert(j.size() == VERTEX_INFLUENCE_COUNT);
    assert(w.size() == VERTEX_INFLUENCE_COUNT);

#if _DEBUG
    float wTotal = 0.0f;
#endif
    for (int i = 0; i < VERTEX_INFLUENCE_COUNT; ++i) {
      skinData.jnts[id + i] = j[i].get<int>();
      skinData.weights[id + i] = w[i].get<float>();

// checking that the weights don't exceed one
#if _DEBUG
      wTotal += skinData.weights[id + i];
#endif
    }

#if _DEBUG
    bool res = (wTotal <= (1.0f + SKIN_TOTAL_WEIGHT_TOLLERANCE));
    assert(res);
#endif
    ++counter;
  }
  return true;
}

// TODO decide what to do with object with no tangent at this point every
// object has tangent and this should be changed
void convertObjNoTangents(const tinyobj::attrib_t &attr,
                          const tinyobj::shape_t &shape, Model &model,
                          const std::string &) {

  // TODO need to add skinPath usage
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
      c.p.x = attr.vertices[3u * idx.vertex_index + 0];
      c.p.y = attr.vertices[3u * idx.vertex_index + 1];
      c.p.z = attr.vertices[3u * idx.vertex_index + 2];
      c.n.x = attr.normals[3u * idx.normal_index + 0];
      c.n.y = attr.normals[3u * idx.normal_index + 1];
      c.n.z = attr.normals[3u * idx.normal_index + 2];
      c.uv.x = attr.texcoords[2u * idx.texcoord_index + 0];
      c.uv.y = attr.texcoords[2u * idx.texcoord_index + 1];
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
  const size_t indicesCount = indices.size();
  const size_t vertexCompareCount = vertexData.size();
  const uint32_t stride = 12;

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
                Model &model, SkinData &finalSkinData,
                const std::string &tangentsPath, const std::string &skinPath) {

  if (tangentsPath.empty()) {
    convertObjNoTangents(attr, shape, model, skinPath);
    return;
  }

  // here we need to process a model which has tangents
  std::vector<float> tangents = loadTangents(tangentsPath);
  SkinData skinData;
  bool hasSkin = loadSkin(skinPath, skinData);
  if (hasSkin) {
    finalSkinData.jnts.reserve(skinData.jnts.size());
    finalSkinData.weights.reserve(skinData.weights.size());
  }

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

  float minX = std::numeric_limits<float>::max();
  float minY = minX;
  float minZ = minY;

  float maxX = std::numeric_limits<float>::min();
  float maxY = maxX;
  float maxZ = maxY;

  for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
    size_t fv = shape.mesh.num_face_vertices[f];
    assert(fv == 3);

    // Loop over vertices in the face.
    for (size_t v = 0; v < fv; v++) {
      // access to vertex
      tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
      VertexCompare c;
      c.p.x = attr.vertices[3u * idx.vertex_index + 0u];
      c.p.y = attr.vertices[3u * idx.vertex_index + 1u];
      c.p.z = attr.vertices[3u * idx.vertex_index + 2u];
      c.n.x = attr.normals[3u * idx.normal_index + 0u];
      c.n.y = attr.normals[3u * idx.normal_index + 1u];
      c.n.z = attr.normals[3u * idx.normal_index + 2u];

      // lets us compute bounding box
      minX = c.p.x < minX ? c.p.x : minX;
      minY = c.p.y < minY ? c.p.y : minY;
      minZ = c.p.z < minZ ? c.p.z : minZ;

      maxX = c.p.x > maxX ? c.p.x : maxX;
      maxY = c.p.y > maxY ? c.p.y : maxY;
      maxZ = c.p.z > maxZ ? c.p.z : maxZ;

      // we had cases where the UV index was -1!! so let us check against that
      float texU = idx.texcoord_index < 0
                       ? 0.0f
                       : attr.texcoords[2 * idx.texcoord_index + 0];
      float texV = idx.texcoord_index < 0
                       ? 0.0f
                       : attr.texcoords[2 * idx.texcoord_index + 1];
      float mod = 1.0f;
      c.uv.x = std::modf(texU, &mod);
      c.uv.y = std::modf(texV, &mod);
      c.t.x = tangents[3u * idx.vertex_index + 0u];
      c.t.y = tangents[3u * idx.vertex_index + 1u];
      c.t.z = tangents[3u * idx.vertex_index + 2u];

      // if the vertex is not in the map, it means is unique
      // and needs to be added and is a valid vertex in the vertex buffer
      if (uniqueVertices.count(c) == 0) {
        uniqueVertices[c] = indexCount;
        vertexData.push_back(c);
        // need to push the skin if we have one
        if (hasSkin) {
          for (int subI = 0; subI < VERTEX_INFLUENCE_COUNT; ++subI) {
            int id = (VERTEX_INFLUENCE_COUNT * idx.vertex_index) + subI;
            finalSkinData.jnts.push_back(skinData.jnts[id]);
            finalSkinData.weights.push_back(skinData.weights[id]);
          }
        }
        // incrementing the counter
        ++indexCount;
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

  model.boundingBox[0] = minX;
  model.boundingBox[1] = minY;
  model.boundingBox[2] = minZ;
  model.boundingBox[3] = maxX;
  model.boundingBox[4] = maxY;
  model.boundingBox[5] = maxZ;
}
