#pragma once
#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

#include "nlohmann/json_fwd.hpp"
#include <cassert>

// forward declares
struct ID3D12Device;
struct ID3D12Resource;

namespace SirEngine {
namespace dx12 {
class DescriptorHeap;
struct Dx12RaytracingMesh;

class ModelManager {

  ModelManager() = default;
  ModelManager(const ModelManager &) = delete;
  ModelManager &operator=(const ModelManager &) = delete;

public:
  virtual ~ModelManager();
  void init(ID3D12Device *device);
  void cleanup();
  void loadMeshObj(const char *path, DescriptorHeap *heap);

  inline Dx12RaytracingMesh *getMeshFromName(const std::string &name) {
    auto found = m_meshRegister.find(name);
    if (found != m_meshRegister.end()) {
      return found->second;
    }
    assert(0 && "could not find mesh in register");
    return nullptr;
  }
  inline int getMeshCount() const {
    return static_cast<int>(m_meshOrder.size());
  }
  inline Dx12RaytracingMesh *getMeshFromindex(int idx) {
    assert(idx < m_meshOrder.size());
    return m_meshOrder[idx];
  }

  void clear();

private:
  ID3D12Device *m_device = nullptr;
  std::unordered_map<std::string, Dx12RaytracingMesh *> m_meshRegister;
  std::vector<Dx12RaytracingMesh *> m_meshOrder;
};
} // namespace dx12
} // namespace SirEngine
