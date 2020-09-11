#include "SirEngine/materialManager.h"

#include <SPIRV-CROSS/spirv_cross.hpp>
#include <cassert>
#include <string>
#include <unordered_map>

#include "SirEngine/PSOManager.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/rootSignatureManager.h"
#include "SirEngine/textureManager.h"
#include "memory/cpu/stringPool.h"
#include "platform/windows/graphics/vk/vkShaderCompiler.h"
#include "runtimeString.h"

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
        {"forwardPhongPSO", SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG},
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

static void parseQueueTypeFlags2(const char **outFlags,
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
    const auto stringFlag = qjobj[i].get<std::string>();
    const uint32_t currentFlag = stringToActualQueueFlag(stringFlag);
    int currentFlagId = static_cast<int>(log2(currentFlag & -currentFlag));

    const auto stringType = tjobj[i].get<std::string>();
    outFlags[currentFlagId] = frameString(stringType.c_str());
  }
}

MaterialManager::PreliminaryMaterialParse MaterialManager::parseMaterial(
    const char *path, const MeshHandle, const SkinHandle skinHandle) {
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

  PreliminaryMaterialParse toReturn;
  toReturn.mat = mat;
  toReturn.handles = texHandles;
  toReturn.isStatic = isStatic;
  parseQueueTypeFlags2(toReturn.shaderQueueTypeFlagsStr, jobj);

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
  return;
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
static const std::string PSO_TYPE_KEY = "type";
static const std::string PSO_TYPE_RASTER = "RASTER";
static const std::string PSO_TYPE_COMPUTE = "COMPUTE";
static const std::string PSO_VS_KEY = "VS";
static const std::string PSO_PS_KEY = "PS";
static const std::string PSO_CS_KEY = "shaderName";
static const std::string DEFAULT_STRING = "";

MaterialMetadata processShader(const char *shaderName, SHADER_TYPE type) {
  GRAPHIC_RESOURCE_VISIBILITY visibility = 0;
  switch (type) {
    case SHADER_TYPE::VERTEX: {
      visibility = GRAPHICS_RESOURCE_VISIBILITY_VERTEX;
      break;
    }
    case SHADER_TYPE::FRAGMENT: {
      visibility = GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT;
      break;
    }
    case SHADER_TYPE::COMPUTE: {
      visibility = GRAPHICS_RESOURCE_VISIBILITY_COMPUTE;
      break;
    }
  }
  assert(visibility && "wrong visibility found for shader type");

  vk::VkShaderCompiler vkCompiler;
  std::string log;
  vk::SpirVBlob blob =
      vkCompiler.compileToSpirV(shaderName, vk::VkShaderArgs{true, type}, &log);

  std::vector<unsigned int> spirV;
  spirV.resize(blob.sizeInByte / 4);
  memcpy(spirV.data(), blob.memory, blob.sizeInByte);
  spirv_cross::Compiler comp(spirV);

  spirv_cross::ShaderResources res = comp.get_shader_resources();

  int totalCount = res.separate_images.size() + res.storage_buffers.size() +
                   res.uniform_buffers.size();
  uint32_t allocSize = sizeof(MaterialResource) * totalCount;
  auto *memory = static_cast<MaterialResource *>(
      globals::FRAME_ALLOCATOR->allocate(allocSize));

  int counter = 0;
  // SE_CORE_INFO("Images");
  for (const auto &image : res.separate_images) {
    auto set = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationDescriptorSet));
    auto binding = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationBinding));
    // SE_CORE_INFO("--> name: {}, set: {}, bind:{}", image.name, set, binding);

    memory[counter++] = {MATERIAL_RESOURCE_TYPE::TEXTURE, visibility, set,
                         binding, frameString(image.name.c_str())};
  }

  // SE_CORE_INFO("ssbo");
  for (const auto &image : res.storage_buffers) {
    auto set = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationDescriptorSet));
    auto binding = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationBinding));
    // SE_CORE_INFO("--> name: {}, set: {}, bind:{}", image.name, set, binding);
    memory[counter++] = {MATERIAL_RESOURCE_TYPE::BUFFER, visibility, set,
                         binding, frameString(image.name.c_str())};
  }

  // SE_CORE_INFO("constant buffers");
  for (const auto &image : res.uniform_buffers) {
    auto set = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationDescriptorSet));
    auto binding = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationBinding));
    // SE_CORE_INFO("--> name: {}, set: {}, bind:{}", image.name, set, binding);
    memory[counter++] = {MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER, visibility,
                         set, binding, frameString(image.name.c_str())};
  }
  return {memory, nullptr, nullptr, static_cast<uint32_t>(totalCount), 0, 0};
}

MaterialMetadata processComputeMetadata(const nlohmann::json &jobj) {
  assertInJson(jobj, PSO_CS_KEY);

  const std::string &name = getValueIfInJson(jobj, PSO_CS_KEY, DEFAULT_STRING);
  const std::string path = "../data/shaders/VK/compute/" + name + ".glsl";
  assert(fileExists(path));
  MaterialMetadata meta = processShader(path.c_str(), SHADER_TYPE::COMPUTE);

  // let us merge
  MaterialResource space0[16];
  MaterialResource space2[16];
  MaterialResource space3[32];
  MaterialResource *resources[4] = {&space0[0], nullptr, &space2[0],
                                    &space3[0]};
  int counters[4] = {0, 0, 0, 0};
  int maxCounters[4] = {16, 0, 16, 32};
  for (uint32_t i = 0; i < meta.objectResourceCount; ++i) {
    MaterialResource &res = meta.objectResources[i];
    res.name = persistentString(res.name);
    resources[res.set][counters[res.set]++] = res;
    assert(counters[res.set] < maxCounters[res.set]);
  }
  std::sort(space0, space0 + counters[0],
            [](const MaterialResource &lhs, const MaterialResource &rhs) {
              return lhs.binding < rhs.binding;
            });
  std::sort(space2, space2 + counters[2],
            [](const MaterialResource &lhs, const MaterialResource &rhs) {
              return lhs.binding < rhs.binding;
            });
  std::sort(space3, space3 + counters[3],
            [](const MaterialResource &lhs, const MaterialResource &rhs) {
              return lhs.binding < rhs.binding;
            });
  MaterialMetadata toReturn{};
  toReturn.frameResources =
      static_cast<MaterialResource *>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(MaterialResource) * counters[0]));
  toReturn.frameResourceCount = counters[0];
  toReturn.passResources =
      static_cast<MaterialResource *>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(MaterialResource) * counters[2]));
  toReturn.passResourceCount = counters[2];
  toReturn.objectResources =
      static_cast<MaterialResource *>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(MaterialResource) * counters[3]));
  toReturn.objectResourceCount = counters[3];
  memcpy(toReturn.frameResources, space0,
         sizeof(MaterialResource) * counters[0]);
  memcpy(toReturn.passResources, space2,
         sizeof(MaterialResource) * counters[2]);
  memcpy(toReturn.objectResources, space3,
         sizeof(MaterialResource) * counters[3]);
  return toReturn;
}
std::string loadFile(const char *path) {
  std::ifstream st(path);
  std::stringstream sBuffer;
  sBuffer << st.rdbuf();
  return sBuffer.str();
}

MaterialMetadata processRasterMetadata(const nlohmann::json &jobj) {
  assertInJson(jobj, PSO_VS_KEY);

  const std::string &vsName =
      getValueIfInJson(jobj, PSO_VS_KEY, DEFAULT_STRING);
  const std::string &psName =
      getValueIfInJson(jobj, PSO_PS_KEY, DEFAULT_STRING);
  const std::string vsPath =
      "../data/shaders/VK/rasterization/" + vsName + ".glsl";
  assert(fileExists(vsPath));
  MaterialMetadata vsMeta = processShader(vsPath.c_str(), SHADER_TYPE::VERTEX);

  MaterialMetadata psMeta;
  if (!psName.empty()) {
    const std::string psPath =
        "../data/shaders/VK/rasterization/" + psName + ".glsl";
    assert(fileExists(psPath));
    psMeta = processShader(psPath.c_str(), SHADER_TYPE::FRAGMENT);
  }

  // let us merge
  MaterialResource space0[16];
  MaterialResource space2[16];
  MaterialResource space3[32];
  MaterialResource *resources[4] = {&space0[0], nullptr, &space2[0],
                                    &space3[0]};
  int counters[4] = {0, 0, 0, 0};
  int maxCounters[4] = {16, 0, 16, 32};
  for (uint32_t i = 0; i < vsMeta.objectResourceCount; ++i) {
    MaterialResource &res = vsMeta.objectResources[i];
    res.name = persistentString(res.name);
    resources[res.set][counters[res.set]++] = res;
    assert(counters[res.set] < maxCounters[res.set]);
  }
  // merging in ps
  if (!psName.empty()) {
    for (uint32_t i = 0; i < psMeta.objectResourceCount; ++i) {
      MaterialResource &res = psMeta.objectResources[i];
      // check if is unique
      int currCounter = maxCounters[res.set];
      bool skip = false;
      for (uint32_t x = 0; x < currCounter; ++x) {
        auto &currRes = resources[res.set][x];
        if (currRes.set == res.set && currRes.binding == res.binding) {
          skip = true;
          currRes.visibility |= GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT;
          break;
        }
      }
      if (skip) {
        continue;
      }
      res.name = persistentString(res.name);
      resources[res.set][counters[res.set]++] = res;
      assert(counters[res.set] < maxCounters[res.set]);
    }
  }
  std::sort(space0, space0 + counters[0],
            [](const MaterialResource &lhs, const MaterialResource &rhs) {
              return lhs.binding < rhs.binding;
            });
  std::sort(space2, space2 + counters[2],
            [](const MaterialResource &lhs, const MaterialResource &rhs) {
              return lhs.binding < rhs.binding;
            });
  std::sort(space3, space3 + counters[3],
            [](const MaterialResource &lhs, const MaterialResource &rhs) {
              return lhs.binding < rhs.binding;
            });
  MaterialMetadata toReturn{};
  toReturn.frameResources =
      static_cast<MaterialResource *>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(MaterialResource) * counters[0]));
  toReturn.frameResourceCount = counters[0];
  toReturn.passResources =
      static_cast<MaterialResource *>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(MaterialResource) * counters[2]));
  toReturn.passResourceCount = counters[2];
  toReturn.objectResources =
      static_cast<MaterialResource *>(globals::PERSISTENT_ALLOCATOR->allocate(
          sizeof(MaterialResource) * counters[3]));
  toReturn.objectResourceCount = counters[3];
  memcpy(toReturn.frameResources, space0,
         sizeof(MaterialResource) * counters[0]);
  memcpy(toReturn.passResources, space2,
         sizeof(MaterialResource) * counters[2]);
  memcpy(toReturn.objectResources, space3,
         sizeof(MaterialResource) * counters[3]);
  return toReturn;
}

MaterialMetadata extractMetadata(const char *psoPath) {
  auto jobj = getJsonObj(psoPath);
  assertInJson(jobj, PSO_TYPE_KEY);

  const std::string &psoType = jobj[PSO_TYPE_KEY].get<std::string>();
  if (psoType == PSO_TYPE_RASTER) {
    auto t = processRasterMetadata(jobj);
    return t;
  }
  return processComputeMetadata(jobj);
}

}  // namespace SirEngine
