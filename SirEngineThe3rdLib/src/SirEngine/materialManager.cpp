#include "SirEngine/materialManager.h"

#include <cassert>
#include <string>
#include <unordered_map>

#include "SirEngine/PSOManager.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/rootSignatureManager.h"
#include "SirEngine/textureManager.h"

namespace SirEngine {
namespace materialKeys {
static const char *KD = "kd";
static const char *KS = "ks";
static const char *KA = "ka";
static const char *SHINESS = "shiness";
static const char *ALBEDO = "albedo";
static const char *NORMAL = "normal";
static const char *METALLIC = "metallic";
static const char *ROUGHNESS = "roughness";
static const char *HEIGHT = "height";
static const char *AO = "ao";
static const char *SEPARATE_ALPHA = "separateAlpha";
static const char *ROUGHNESS_MULT = "roughnessMult";
static const char *METALLIC_MULT = "metallicMult";
static const char *THICKNESS = "thickness";
static const char *QUEUE = "queue";
static const char *TYPE = "type";
static const char *RS_KEY = "rs";
static const char *PSO_KEY = "pso";
static const char *IS_STATIC_KEY = "isStatic";
static const std::string DEFAULT_STRING = "";

static const std::unordered_map<std::string, SirEngine::SHADER_QUEUE_FLAGS>
    STRING_TO_QUEUE_FLAG{
        {"forward", SirEngine::SHADER_QUEUE_FLAGS::FORWARD},
        {"deferred", SirEngine::SHADER_QUEUE_FLAGS::DEFERRED},
        {"shadow", SirEngine::SHADER_QUEUE_FLAGS::SHADOW},
        {"debug", SirEngine::SHADER_QUEUE_FLAGS::QUEUE_DEBUG},
        {"custom", SirEngine::SHADER_QUEUE_FLAGS::CUSTOM},
    };
static const std::unordered_map<std::string, SirEngine::SHADER_TYPE_FLAGS>
    STRING_TO_TYPE_FLAGS{
        {"pbr", SirEngine::SHADER_TYPE_FLAGS::PBR},
        {"forwardPbr", SirEngine::SHADER_TYPE_FLAGS::FORWARD_PBR},
        {"forwardPhong", SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG},
        {"forwardPhongAlphaCutout",
         SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT},
        {"skin", SirEngine::SHADER_TYPE_FLAGS::SKIN},
        {"hair", SirEngine::SHADER_TYPE_FLAGS::HAIR},
        {"hairSkin", SirEngine::SHADER_TYPE_FLAGS::HAIRSKIN},
        {"skinCluster", SirEngine::SHADER_TYPE_FLAGS::SKINCLUSTER},
        {"skinSkinCluster", SirEngine::SHADER_TYPE_FLAGS::SKINSKINCLUSTER},
        {"forwardPhongAlphaCutoutSkin",
         SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT_SKIN},
        {"forwardParallax", SirEngine::SHADER_TYPE_FLAGS::FORWARD_PARALLAX},
        {"shadowSkinCluster",
         SirEngine::SHADER_TYPE_FLAGS::SHADOW_SKIN_CLUSTER},
    };
static const std::unordered_map<SirEngine::SHADER_TYPE_FLAGS, std::string>
    TYPE_FLAGS_TO_STRING{
        {SirEngine::SHADER_TYPE_FLAGS::PBR, "pbr"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PBR, "forwardPbr"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG, "forwardPhong"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT,
         "forwardPhongAlphaCutout"},
        {SirEngine::SHADER_TYPE_FLAGS::SKIN, "skin"},
        {SirEngine::SHADER_TYPE_FLAGS::HAIR, "hair"},
        {SirEngine::SHADER_TYPE_FLAGS::HAIRSKIN, "hairSkin"},
        {SirEngine::SHADER_TYPE_FLAGS::SKINCLUSTER, "skinCluster"},
        {SirEngine::SHADER_TYPE_FLAGS::SKINSKINCLUSTER, "skinSkinCluster"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT_SKIN,
         "forwardPhongAlphaCutoutSkin"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PARALLAX, "forwardParallax"},
        {SirEngine::SHADER_TYPE_FLAGS::SHADOW_SKIN_CLUSTER,
         "shadowSkinCluster"},
    };

}  // namespace materialKeys

inline uint32_t stringToActualQueueFlag(const std::string &flag) {
  const auto found = materialKeys::STRING_TO_QUEUE_FLAG.find(flag);
  if (found != materialKeys::STRING_TO_QUEUE_FLAG.end()) {
    return static_cast<uint32_t>(found->second);
  }
  assert(0 && "could not map requested queue flag");
  return 0;
}
inline uint16_t stringToActualTypeFlag(const std::string &flag) {
  const auto found = materialKeys::STRING_TO_TYPE_FLAGS.find(flag);
  if (found != materialKeys::STRING_TO_TYPE_FLAGS.end()) {
    return static_cast<uint16_t>(found->second);
  }
  assert(0 && "could not map requested type flag");
  return 0;
}

uint16_t MaterialManager::parseTypeFlags(const char *stringType) {
  const uint16_t typeFlag = stringToActualTypeFlag(stringType);
  return typeFlag;
}

static void parseQueueTypeFlags(uint32_t *outFlags,
                                const nlohmann::json &jobj) {
  if (jobj.find(materialKeys::QUEUE) == jobj.end()) {
    assert(0 && "cannot find queue flags in material");
    return;
  }
  if (jobj.find(materialKeys::TYPE) == jobj.end()) {
    assert(0 && "cannot find types in material");
    return;
  }

  // extract the queue flags, that flag is a bit field for an
  // uint16, which is merged with the material type which
  // is a normal increasing uint16
  const auto &qjobj = jobj[materialKeys::QUEUE];
  const auto &tjobj = jobj[materialKeys::TYPE];
  assert(qjobj.size() == tjobj.size());

  for (size_t i = 0; i < qjobj.size(); ++i) {
    uint32_t flags = 0;
    const auto stringFlag = qjobj[i].get<std::string>();
    const uint32_t currentFlag = stringToActualQueueFlag(stringFlag);
    flags |= currentFlag;

    int currentFlagId = static_cast<int>(log2(currentFlag & -currentFlag));

    const auto stringType = tjobj[i].get<std::string>();
    const uint32_t typeFlag =
        MaterialManager::parseTypeFlags(stringType.c_str());
    flags = typeFlag << 16 | flags;

    outFlags[currentFlagId] = flags;
  }
}

MaterialManager::PreliminaryMaterialParse MaterialManager::parseMaterial(
    const char *path, const MeshHandle meshHandle,
    const SkinHandle skinHandle) {
  // for materials we do not perform the check whether is loaded or not
  // each object is going to get it s own material copy.
  // if that starts to be an issue we will add extra logic to deal with this.
  assert(fileExists(path));
  const std::string name = getFileName(path);

  auto jobj = getJsonObj(path);
  glm::vec4 zero{0.0f, 0.0f, 0.0f, 0.0f};
  glm::vec4 kd = getValueIfInJson(jobj, materialKeys::KD, zero);
  glm::vec4 ka = getValueIfInJson(jobj, materialKeys::KA, zero);
  glm::vec4 ks = getValueIfInJson(jobj, materialKeys::KS, zero);
  float zeroFloat = 0.0f;
  float oneFloat = 1.0f;
  float shininess = getValueIfInJson(jobj, materialKeys::SHINESS, zeroFloat);
  float roughnessMult =
      getValueIfInJson(jobj, materialKeys::ROUGHNESS_MULT, oneFloat);
  float metallicMult =
      getValueIfInJson(jobj, materialKeys::METALLIC_MULT, oneFloat);
  bool isStatic = getValueIfInJson(jobj, materialKeys::IS_STATIC_KEY, false);

  const std::string empty;
  const std::string albedoName =
      getValueIfInJson(jobj, materialKeys::ALBEDO, empty);
  const std::string normalName =
      getValueIfInJson(jobj, materialKeys::NORMAL, empty);
  const std::string metallicName =
      getValueIfInJson(jobj, materialKeys::METALLIC, empty);
  const std::string roughnessName =
      getValueIfInJson(jobj, materialKeys::ROUGHNESS, empty);
  const std::string thicknessName =
      getValueIfInJson(jobj, materialKeys::THICKNESS, empty);
  const std::string separateAlphaName =
      getValueIfInJson(jobj, materialKeys::SEPARATE_ALPHA, empty);
  const std::string heightName =
      getValueIfInJson(jobj, materialKeys::HEIGHT, empty);
  const std::string aoName = getValueIfInJson(jobj, materialKeys::AO, empty);

  TextureHandle albedoTex{0};
  TextureHandle normalTex{0};
  TextureHandle metallicTex{0};
  TextureHandle roughnessTex{0};
  TextureHandle thicknessTex{0};
  TextureHandle separateAlphaTex{0};
  TextureHandle heightTex{0};
  TextureHandle aoTex{0};

  if (!albedoName.empty()) {
    albedoTex = globals::TEXTURE_MANAGER->loadTexture(albedoName.c_str());
  } else {
    albedoTex = globals::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!normalName.empty()) {
    normalTex = globals::TEXTURE_MANAGER->loadTexture(normalName.c_str());
  } else {
    normalTex = globals::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!metallicName.empty()) {
    metallicTex = globals::TEXTURE_MANAGER->loadTexture(metallicName.c_str());
  } else {
    metallicTex = globals::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!roughnessName.empty()) {
    roughnessTex = globals::TEXTURE_MANAGER->loadTexture(roughnessName.c_str());
  } else {
    roughnessTex = globals::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!thicknessName.empty()) {
    thicknessTex = globals::TEXTURE_MANAGER->loadTexture(thicknessName.c_str());
  } else {
    thicknessTex = globals::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!separateAlphaName.empty()) {
    separateAlphaTex =
        globals::TEXTURE_MANAGER->loadTexture(separateAlphaName.c_str());
  } else {
    separateAlphaTex = globals::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!aoName.empty()) {
    aoTex = globals::TEXTURE_MANAGER->loadTexture(aoName.c_str());
  } else {
    aoTex = globals::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!heightName.empty()) {
    heightTex = globals::TEXTURE_MANAGER->loadTexture(heightName.c_str());
  } else {
    heightTex = globals::TEXTURE_MANAGER->getWhiteTexture();
  }

  Material mat{};
  mat.kDR = kd.x;
  mat.kDG = kd.y;
  mat.kDB = kd.z;
  mat.kAR = ka.x;
  mat.kAG = ka.y;
  mat.kAB = ka.z;
  mat.kSR = ks.x;
  mat.kSG = ks.y;
  mat.kSB = ks.z;
  mat.shiness = shininess;
  mat.roughnessMult = roughnessMult;
  mat.metallicMult = metallicMult;

  MaterialDataHandles texHandles{};
  texHandles.albedo = albedoTex;
  texHandles.normal = normalTex;
  texHandles.metallic = metallicTex;
  texHandles.roughness = roughnessTex;
  texHandles.thickness = thicknessTex;
  texHandles.separateAlpha = separateAlphaTex;
  texHandles.ao = aoTex;
  texHandles.skinHandle = skinHandle;
  texHandles.height = heightTex;

  uint32_t shaderQueueTypeFlags;
  PreliminaryMaterialParse toReturn;
  toReturn.mat = mat;
  toReturn.handles = texHandles;
  toReturn.isStatic = isStatic;
  parseQueueTypeFlags(toReturn.shaderQueueTypeFlags, jobj);

  return toReturn;
}

const char *MaterialManager::getStringFromShaderTypeFlag(
    const SHADER_TYPE_FLAGS type) {
  const auto found = materialKeys::TYPE_FLAGS_TO_STRING.find(type);
  if (found != materialKeys::TYPE_FLAGS_TO_STRING.end()) {
    return found->second.c_str();
  }

  assert(0 && "Could not find flag");
  const auto unknown =
      materialKeys::TYPE_FLAGS_TO_STRING.find(SHADER_TYPE_FLAGS::UNKNOWN);
  return unknown->second.c_str();
}
void MaterialManager::loadTypeFile(const char *path) {
  const auto jObj = getJsonObj(path);
  SE_CORE_INFO("[Engine]: Loading Material Type from: {0}", path);

  const std::string rsString = getValueIfInJson(jObj, materialKeys::RS_KEY,
                                                materialKeys::DEFAULT_STRING);
  const std::string psoString = getValueIfInJson(jObj, materialKeys::PSO_KEY,
                                                 materialKeys::DEFAULT_STRING);

  assert(!rsString.empty() && "root signature is emtpy in material type");
  assert(!psoString.empty() && "pso  is emtpy in material type");

  // get the handles
  const PSOHandle psoHandle =
      globals::PSO_MANAGER->getHandleFromName(psoString.c_str());
  const RSHandle rsHandle =
      globals::ROOT_SIGNATURE_MANAGER->getHandleFromName(rsString.c_str());

  std::string name = getFileName(path);

  const std::string type = jObj[materialKeys::TYPE].get<std::string>();
  const uint16_t flags = MaterialManager::parseTypeFlags(type.c_str());
  m_shaderTypeToShaderBind.insert(flags, ShaderBind{rsHandle, psoHandle});
}

void MaterialManager::loadTypesInFolder(const char *folder) {
  std::vector<std::string> paths;
  listFilesInFolder(folder, paths, "json");

  for (const auto &p : paths) {
    loadTypeFile(p.c_str());
  }
}

}  // namespace SirEngine
