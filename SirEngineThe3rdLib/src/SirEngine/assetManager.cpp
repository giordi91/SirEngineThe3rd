#include "SirEngine/assetManager.h"
#include "SirEngine/animation/animationManager.h"
#include "fileUtils.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "SirEngine/textureManager.h"
#include "platform/windows/graphics/dx12/dx12MaterialManager.h"
#include "skinClusterManager.h"
#include "SirEngine/meshManager.h"

namespace SirEngine {
namespace AssetManagerKeys {
static const char *MESH_KEY = "mesh";
static const char *MATERIAL_KEY = "material";
static const char *SKIN_KEY = "skin";
static const char *ANIM_CONFIG_KEY = "animConfig";
static const char *ASSETS_KEY = "assets";
static const char *SUB_ASSETS_KEY = "subAssets";
static const char *ENVIROMENT_MAP_KEY = "enviromentMap";
static const char *ENVIROMENT_MAP_IRRADIANCE_KEY = "enviromentMapIrradiance";
static const char *ENVIROMENT_MAP_RADIANCE_KEY = "enviromentMapRadiance";
static const std::string DEFAULT_STRING = "";
} // namespace AssetManagerKeys

AssetDataHandle AssetManager::loadAsset(const char *path) {
  auto jobj = getJsonObj(path);

  const std::string assetName = getFileName(path);
  // now that we have the asset we can check that the sub asset is present

  assert(jobj.find(AssetManagerKeys::SUB_ASSETS_KEY) != jobj.end());
  auto subAssetsJ = jobj[AssetManagerKeys::SUB_ASSETS_KEY];

  for (auto &subAsset : subAssetsJ) {

    Renderable renderable{};
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
    MeshHandle mHandle = globals::MESH_MANAGER->loadMesh(meshString.c_str());
    renderable.m_meshHandle = mHandle;

    // load animation if present
    const std::string animConfigPath =
        getValueIfInJson(subAsset, AssetManagerKeys::ANIM_CONFIG_KEY,
                         AssetManagerKeys::DEFAULT_STRING);
    AnimationConfigHandle animHandle{0};
    if (!animConfigPath.empty()) {
      animHandle = globals::ANIMATION_MANAGER->loadAnimationConfig(
          animConfigPath.c_str(), assetName.c_str());
    }

    // load skin if present
    const std::string skinPath = getValueIfInJson(
        subAsset, AssetManagerKeys::SKIN_KEY, AssetManagerKeys::DEFAULT_STRING);
    SkinHandle skinHandle{0};
    if (!skinPath.empty()) {
      skinHandle =
          globals::SKIN_MANAGER->loadSkinCluster(skinPath.c_str(), animHandle);
    }
    MaterialHandle matHandle = dx12::MATERIAL_MANAGER->loadMaterial(
        materialString.c_str(), mHandle,skinHandle);
    renderable.m_materialHandle = matHandle;

    globals::RENDERING_CONTEXT->addRenderablesToQueue(renderable);
  }

  // not currently using this handle, need a way to identify assets, for now the
  // engine has no concept of asset, in the meaning of a conglomerate of
  // data,meshes,textures,animations and so on
  return AssetDataHandle{};
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

  dx12::RENDERING_CONTEXT->setEnviromentMap(enviromentMapHandle);
  dx12::RENDERING_CONTEXT->setEnviromentMapIrradiance(
      enviromentMapIrradianceHandle);
  dx12::RENDERING_CONTEXT->setEnviromentMapRadiance(
      enviromentMapRadianceHandle);

  TextureHandle brdfHandle = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/brdf.texture");
  dx12::RENDERING_CONTEXT->setBrdfHandle(brdfHandle);
}
} // namespace SirEngine
