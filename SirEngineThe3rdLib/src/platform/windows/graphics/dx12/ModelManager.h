#pragma once
#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

#include <cassert>
	
// forward declares
struct ID3D12Device;
struct ID3D12Resource;

namespace SirEngine {
namespace dx12 {
class DescriptorHeap;
struct Mesh;

class ModelManager {

  ModelManager() = default;
  ModelManager(const ModelManager &) = delete;
  ModelManager &operator=(const ModelManager &) = delete;

public:
  virtual ~ModelManager();
  void init(ID3D12Device *device);
  void cleanup();
  void loadMeshObj(const char *path, DescriptorHeap *heap);

  inline Mesh *getMeshFromName(const std::string &name) {
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
  inline Mesh *getMeshFromindex(int idx) {
    assert(idx < m_meshOrder.size());
    return m_meshOrder[idx];
  }

  void clear();

private:
  ID3D12Device *m_device = nullptr;
  std::unordered_map<std::string, Mesh *> m_meshRegister;
  std::vector<Mesh *> m_meshOrder;
};
} // namespace dx12
} // namespace SirEngine
