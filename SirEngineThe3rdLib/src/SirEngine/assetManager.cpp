#include "SirEngine/assetManager.h"
#include "fileUtils.h"
// TODO remove dx12 calls from here
#include "SirEngine/identityManager.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/meshManager.h"

namespace SirEngine {
namespace AssetManagerKeys {
static const char *MESH_KEY = "mesh";
static const char *MATERIAL_KEY = "material";
static const char *ASSETS_KEY = "assets";
static const char *ENVIROMENT_MAP_KEY = "enviromentMap";
static const char *ENVIROMENT_MAP_IRRADIANCE_KEY = "enviromentMapIrradiance";
static const std::string DEFAULT_STRING = "";
} // namespace AssetManagerKeys

AssetManager::AssetManager() {}

bool AssetManager::initialize() {
  m_meshRuntime.resize(DATA_SIZE_ALLOC);
  m_assetHandles.resize(DATA_SIZE_ALLOC);
  m_materialRuntime.resize(DATA_SIZE_ALLOC);
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

  uint32_t currIdx = allocIndex++;
  // first of all generate an identity handle for the asset
  const std::string &fileName = getFileName(path);
  IdentityHandle id =
      dx12::IDENTITY_MANAGER->getHandleFromName(fileName.c_str());

  // lets load the mesh
  MeshHandle mHandle = dx12::MESH_MANAGER->loadMesh(meshString.c_str(), currIdx,
                                                    m_meshRuntime.data());

  MaterialHandle matHandle = dx12::MATERIAL_MANAGER->loadMaterial(
      materialString.c_str(), currIdx, m_materialRuntime.data());

  // bookkeeping
  AssetHandles assetH;
  assetH.materialH = matHandle;
  assetH.meshH = mHandle;
  m_assetHandles[currIdx] = assetH;

  m_identityToIndex[id.handle] = currIdx;

  return id;
}

void AssetManager::loadScene(const char *path) {
  auto jobj = getJsonObj(path);
  assert(jobj.find(AssetManagerKeys::ASSETS_KEY) != jobj.end());

  // load all the assets
  auto assetsJ = jobj[AssetManagerKeys::ASSETS_KEY];
  for (const auto &asset : assetsJ) {
    // should I keep track of the assets?
    // I suppose I should but for now there is not the use case
    const std::string path = asset.get<std::string>();
    loadAsset(path.c_str());
  }

  // load the env map
  const std::string enviromentMapString =
      getValueIfInJson(jobj, AssetManagerKeys::ENVIROMENT_MAP_KEY,
                       AssetManagerKeys::DEFAULT_STRING);
  assert(!enviromentMapString.empty());
  TextureHandle enviromentMapHandle =
      globals::TEXTURE_MANAGER->loadTexture(enviromentMapString.c_str(), true);

  // load the env map irradiance
  const std::string enviromentMapIrradianceString =
      getValueIfInJson(jobj, AssetManagerKeys::ENVIROMENT_MAP_IRRADIANCE_KEY,
                       AssetManagerKeys::DEFAULT_STRING);
  assert(!enviromentMapIrradianceString.empty());
  TextureHandle enviromentMapIrradianceHandle =
      globals::TEXTURE_MANAGER->loadTexture(enviromentMapIrradianceString.c_str(),true);

	globals::RENDERING_CONTEX->setEnviromentMap(enviromentMapHandle);
	globals::RENDERING_CONTEX->setEnviromentMapIrradiance(enviromentMapIrradianceHandle);
}
} // namespace SirEngine
