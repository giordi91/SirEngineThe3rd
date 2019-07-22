#include "SirEngine/assetManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/identityManager.h"
#include "SirEngine/materialManager.h"
#include "fileUtils.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/meshManager.h"

namespace SirEngine {
namespace AssetManagerKeys {
static const char *MESH_KEY = "mesh";
static const char *MATERIAL_KEY = "material";
static const char *ASSETS_KEY = "assets";
static const char *SUB_ASSETS_KEY = "subAssets";
static const char *ENVIROMENT_MAP_KEY = "enviromentMap";
static const char *ENVIROMENT_MAP_IRRADIANCE_KEY = "enviromentMapIrradiance";
static const char *ENVIROMENT_MAP_RADIANCE_KEY = "enviromentMapRadiance";
static const std::string DEFAULT_STRING = "";
} // namespace AssetManagerKeys

AssetManager::AssetManager() {}

bool AssetManager::initialize() {

  // allocate master handle
  m_masterHandle = StreamHandle{(MAGIC_NUMBER_COUNTER << 16)};
  m_renderables = &m_streamMapper[m_masterHandle.handle];

  return true;
}

IdentityHandle AssetManager::loadAsset(const char *path) {
  auto jobj = getJsonObj(path);

  // now that we have the asset we can check that the sub asset is present

  assert(jobj.find(AssetManagerKeys::SUB_ASSETS_KEY) != jobj.end());
  auto subAssetsJ = jobj[AssetManagerKeys::SUB_ASSETS_KEY];

  for (auto &subAsset : subAssetsJ) {

    Renderable renderable;
    // get the mesh
    const std::string meshString = getValueIfInJson(
        subAsset, AssetManagerKeys::MESH_KEY, AssetManagerKeys::DEFAULT_STRING);
    assert(!meshString.empty());

    // get material
    const std::string materialString =
        getValueIfInJson(subAsset, AssetManagerKeys::MATERIAL_KEY,
                         AssetManagerKeys::DEFAULT_STRING);
    assert(!materialString.empty());

    // lets load the mesh
    MeshHandle mHandle = dx12::MESH_MANAGER->loadMesh(
        meshString.c_str(), &renderable.m_meshRuntime);

    MaterialHandle matHandle = dx12::MATERIAL_MANAGER->loadMaterial(
        materialString.c_str(), &renderable.m_materialRuntime);

    // store the renderable
    (*m_renderables)[renderable.m_materialRuntime.shaderQueueTypeFlags].push_back(
        renderable);
  }

  // TODO identity handle concept is not used and is completely broken since we
  // introduced the sub assets, needs to be fixed or removed
  return IdentityHandle{};
}

void AssetManager::loadScene(const char *path) {
  const auto jobj = getJsonObj(path);
  assert(jobj.find(AssetManagerKeys::ASSETS_KEY) != jobj.end());

  // load all the assets
  auto assetsJ = jobj[AssetManagerKeys::ASSETS_KEY];
  for (const auto &asset : assetsJ) {
    // should I keep track of the assets?
    // I suppose I should but for now there is not the use case
    const auto assetPath = asset.get<std::string>();
    loadAsset(assetPath.c_str());
  }

  // load the env map
  const std::string enviromentMapString =
      getValueIfInJson(jobj, AssetManagerKeys::ENVIROMENT_MAP_KEY,
                       AssetManagerKeys::DEFAULT_STRING);
  assert(!enviromentMapString.empty());
  TextureHandle enviromentMapHandle =
      globals::TEXTURE_MANAGER->loadTexture(enviromentMapString.c_str(), true);

  // load the env map irradiance and radiance
  const std::string enviromentMapIrradianceString =
      getValueIfInJson(jobj, AssetManagerKeys::ENVIROMENT_MAP_IRRADIANCE_KEY,
                       AssetManagerKeys::DEFAULT_STRING);
  assert(!enviromentMapIrradianceString.empty());
  TextureHandle enviromentMapIrradianceHandle =
      globals::TEXTURE_MANAGER->loadTexture(
          enviromentMapIrradianceString.c_str(), true);

  const std::string enviromentMapRadianceString =
      getValueIfInJson(jobj, AssetManagerKeys::ENVIROMENT_MAP_RADIANCE_KEY,
                       AssetManagerKeys::DEFAULT_STRING);
  assert(!enviromentMapIrradianceString.empty());
  TextureHandle enviromentMapRadianceHandle =
      globals::TEXTURE_MANAGER->loadTexture(enviromentMapRadianceString.c_str(),
                                            true);

  globals::RENDERING_CONTEXT->setEnviromentMap(enviromentMapHandle);
  globals::RENDERING_CONTEXT->setEnviromentMapIrradiance(
      enviromentMapIrradianceHandle);
  globals::RENDERING_CONTEXT->setEnviromentMapRadiance(
      enviromentMapRadianceHandle);
}
} // namespace SirEngine
