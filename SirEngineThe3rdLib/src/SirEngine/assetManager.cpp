#include "SirEngine/assetManager.h"
#include "fileUtils.h"
// TODO remove dx12 calls from here
#include "SirEngine/identityManager.h"
#include "SirEngine/materialManager.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/meshManager.h"
#include "platform/windows/graphics/dx12/textureManager.h"

namespace SirEngine {
namespace AssetManagerKeys {
static const char *MESH_KEY = "mesh";
static const char *MATERIAL_KEY = "material";
static const char *ALBEDO_TEX = "albedoTex";
static const std::string DEFAULT_STRING = "";
} // namespace AssetManagerKeys

AssetManager::AssetManager() {}

bool AssetManager::initialize() {
  m_runtimeMeshes.resize(DATA_SIZE_ALLOC);

  m_assetHandles.resize(DATA_SIZE_ALLOC);
  m_materialsCPU.resize(DATA_SIZE_ALLOC);
  m_materials.resize(DATA_SIZE_ALLOC);
  m_materialsMagic.resize(DATA_SIZE_ALLOC);

  m_materialMemory = Materials::MaterialsMemory{&m_materialsCPU, &m_materials,
                                                &m_materialsMagic};
  return true;
}

IdentityHandle AssetManager::loadAsset(const char *path) {
  auto jobj = getJsonObj(path);

  // get the mesh
  const std::string meshString = getValueIfInJson(
      jobj, AssetManagerKeys::MESH_KEY, AssetManagerKeys::DEFAULT_STRING);
  assert(!meshString.empty());

  // get material
  const std::string materialString = getValueIfInJson(
      jobj, AssetManagerKeys::MATERIAL_KEY, AssetManagerKeys::DEFAULT_STRING);
  assert(!materialString.empty());

  // get texture
  const std::string albedoString = getValueIfInJson(
      jobj, AssetManagerKeys::ALBEDO_TEX, AssetManagerKeys::DEFAULT_STRING);
  assert(!albedoString.empty());

  uint32_t currIdx = allocIndex++;
  // first of all generate an identity handle for the asset
  const std::string &fileName = getFileName(path);
  IdentityHandle id =
      dx12::IDENTITY_MANAGER->createHandleFromName(fileName.c_str());

  // lets load the mesh
  MeshHandle mHandle = dx12::MESH_MANAGER->loadMesh(
      meshString.c_str(), currIdx, m_runtimeMeshes.data());

  TextureHandle tHandle =
      dx12::TEXTURE_MANAGER->loadTexture(albedoString.c_str(), false);
  MaterialHandle matHandle =
      // dx12::MATERIAL_MANAGER->loadMaterial(materialString.c_str());
      Materials::loadMaterial(materialString.c_str(), currIdx,
                              m_materialMemory);

  // bookkeeping
  AssetHandles assetH;
  assetH.materialH = matHandle;
  assetH.meshH = mHandle;
  m_assetHandles[currIdx] = assetH;

  m_identityToIndex[id.handle] = currIdx;

  return id;
}
} // namespace SirEngine
