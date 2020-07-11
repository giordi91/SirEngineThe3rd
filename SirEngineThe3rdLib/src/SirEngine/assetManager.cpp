#include "SirEngine/assetManager.h"

#include "SirEngine/animation/animationManager.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/meshManager.h"
#include "SirEngine/runtimeString.h"
#include "SirEngine/skinClusterManager.h"
#include "SirEngine/textureManager.h"
#include "fileUtils.h"

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
}  // namespace AssetManagerKeys

void AssetManager::cleanup() {}

AssetDataHandle AssetManager::loadAsset(const char *path) {
  auto jobj = getJsonObj(path);

  const std::string assetName = getFileName(path);
  // now that we have the asset we can check that the sub asset is present

  assert(jobj.find(AssetManagerKeys::SUB_ASSETS_KEY) != jobj.end());
  auto subAssetsJ = jobj[AssetManagerKeys::SUB_ASSETS_KEY];

  uint32_t subAssetCount = static_cast<uint32_t>(subAssetsJ.size());
  uint32_t assetIdx;
  AssetData &assetData = m_assetDatabase.getFreeMemoryData(assetIdx);
  assetData.m_subAssets = reinterpret_cast<AssetDataHandle *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(AssetDataHandle) *
                                              subAssetCount));
  assetData.magicNumber = MAGIC_NUMBER_COUNTER++;
  assetData.name = persistentString(assetName.c_str());
  AssetDataHandle assetHandle = {assetData.magicNumber << 16 | assetIdx};

  uint32_t assetCounter = 0;
  for (auto &subAsset : subAssetsJ) {
    uint32_t subAssetIdx;
    AssetData &subAssetData = m_assetDatabase.getFreeMemoryData(subAssetIdx);
    subAssetData.m_subAssets = nullptr;
    subAssetData.magicNumber = MAGIC_NUMBER_COUNTER++;
    AssetDataHandle subAssetHandle = {assetData.magicNumber << 16 |
                                      subAssetIdx};

    Renderable renderable{};
    // get the mesh
    const std::string meshString = getValueIfInJson(
        subAsset, AssetManagerKeys::MESH_KEY, AssetManagerKeys::DEFAULT_STRING);
    assert(!meshString.empty());

    // TODO using mesh string as asset name, should have a proper name and then
    // keep track of what it contains for now this will do
    assetData.name = persistentString(meshString.c_str());

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
    MaterialHandle matHandle = globals::MATERIAL_MANAGER->loadMaterial(
        materialString.c_str(), mHandle, skinHandle);
    renderable.m_materialHandle = matHandle;

    globals::RENDERING_CONTEXT->addRenderablesToQueue(renderable);

    // create a handle
    assetData.m_subAssets[assetCounter++] = subAssetHandle;
  }

  // not currently using this handle, need a way to identify assets, for now the
  // engine has no concept of asset, in the meaning of a conglomerate of
  // data,meshes,textures,animations and so on
  return assetHandle;
}

AssetDataHandle AssetManager::loadScene(const char *path) {
  const auto jobj = getJsonObj(path);
  assert(jobj.find(AssetManagerKeys::ASSETS_KEY) != jobj.end());

  // load all the assets
  auto assetsJ = jobj[AssetManagerKeys::ASSETS_KEY];
  uint32_t subAssetCount = static_cast<uint32_t>(assetsJ.size());
  uint32_t assetIdx;
  AssetData &assetData = m_assetDatabase.getFreeMemoryData(assetIdx);
  assetData.m_subAssets = reinterpret_cast<AssetDataHandle *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(AssetDataHandle) *
                                              subAssetCount));
  assetData.magicNumber = MAGIC_NUMBER_COUNTER++;
  assetData.name = persistentString(path);
  AssetDataHandle assetHandle = {assetData.magicNumber << 16 | assetIdx};

  int assetCounter = 0;
  for (const auto &asset : assetsJ) {
    // should I keep track of the assets?
    // I suppose I should but for now there is not the use case
    const auto assetPath = asset.get<std::string>();
    assetData.m_subAssets[assetCounter++] = loadAsset(assetPath.c_str());
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

  TextureHandle brdfHandle = globals::TEXTURE_MANAGER->loadTexture(
      "../data/processed/textures/brdf.texture");
  globals::RENDERING_CONTEXT->setBrdfHandle(brdfHandle);

  return assetHandle;
}
}  // namespace SirEngine
