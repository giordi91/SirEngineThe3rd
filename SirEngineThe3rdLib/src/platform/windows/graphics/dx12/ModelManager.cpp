#include "platform/windows/graphics/dx12/ModelManager.h"
#include "platform/windows/graphics/dx12/mesh.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "SirEngine/fileUtils.h"

namespace SirEngine{
namespace dx12 {

ModelManager::~ModelManager() {}
void ModelManager::init(ID3D12Device *device) { m_device = device; }
void ModelManager::cleanup() {
  for (auto it : m_meshRegister) {
    delete it.second;
  }
}
void ModelManager::loadMeshObj(const char *path, DescriptorHeap *heap) {
  auto *mesh = new Dx12RaytracingMesh();
  mesh->loadFromFile(m_device, path, heap);
  const std::string name = getFileName(path);
  m_meshRegister[name] = mesh;
  m_meshOrder.push_back(mesh);
}

void ModelManager::clear()
{
    m_meshRegister.clear();
    for (auto *m : m_meshOrder) {
      delete m;
    }
	m_meshOrder.clear();
}

} // namespace rendering
} // namespace dx12

