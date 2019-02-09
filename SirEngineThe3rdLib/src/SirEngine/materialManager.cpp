#include "SirEngine/materialManager.h"
#include "SirEngine/IdentityManager.h"
#include "SirEngine/fileUtils.h"
#include "nlohmann/json.hpp"
#include <DirectXMath.h>
#include <unordered_map>

namespace materialKeys {
static const char *KD = "kd";
static const char *KS = "ks";
static const char *KA = "ka";
static const char *ALBEDO = "albedo";
static const char *NORMAL = "normal";
static const char *FLAGS = "flags";
static const std::unordered_map<std::string, SirEngine::SHADER_PASS_FLAGS>
    stringToShaderFlag{{"forward", SirEngine::SHADER_PASS_FLAGS::FORWARD}};

} // namespace materialKeys

namespace SirEngine {
inline uint32_t stringToActualShaderFlag(const std::string &flag) {
  auto found = materialKeys::stringToShaderFlag.find(flag);
  if (found != materialKeys::stringToShaderFlag.end()) {
    return found->second;
  }
  assert(0 && "could not map requested shader flag");
  return 0;
}

uint32_t parseFlags(const nlohmann::json &jobj) {
  if (jobj.find(materialKeys::FLAGS) == jobj.end()) {
    assert(0 && "cannot find flags in material");
    return 0;
  }
  const auto &fjobj = jobj[materialKeys::FLAGS];
  uint32_t flags = 0;
  for (const auto &flag : fjobj) {
    const std::string sFlag = flag.get<std::string>();
    uint32_t currentFlag = stringToActualShaderFlag(sFlag);
    flags |= currentFlag;
  }
  return flags;
}

MaterialHandle MaterialManager::loadMaterial(const char *path) {

  auto jobj = getJsonObj(path);
  DirectX::XMFLOAT4 zero{0.0f, 0.0f, 0.0f, 0.0f};
  DirectX::XMFLOAT4 kd = getValueIfInJson(jobj, materialKeys::KD, zero);
  DirectX::XMFLOAT4 ka = getValueIfInJson(jobj, materialKeys::KA, zero);
  DirectX::XMFLOAT4 ks = getValueIfInJson(jobj, materialKeys::KS, zero);

  const std::string empty;
  const std::string albedoName =
      getValueIfInJson(jobj, materialKeys::ALBEDO, empty);
  const std::string normalName =
      getValueIfInJson(jobj, materialKeys::NORMAL, empty);

  dx12::TextureHandle albedoTex{0};
  dx12::TextureHandle normalTex{0};

  if (!albedoName.empty()) {
    IdentityHandle albedoH =
        dx12::IDENTITY_MANAGER->getHandleFromName(albedoName.c_str());
    albedoTex = dx12::TEXTURE_MANAGER->getHandle(albedoH);

  } else {
    // TODO provide white texture as default;
  }
  if (!normalName.empty()) {
    IdentityHandle normalH =
        dx12::IDENTITY_MANAGER->getHandleFromName(albedoName.c_str());
    normalTex = dx12::TEXTURE_MANAGER->getHandle(normalH);
  } else {
    // TODO provide white texture as default;
  }

  uint32_t flags = parseFlags(jobj);

  Material mat;
  mat.kDR = kd.x;
  mat.kDG = kd.y;
  mat.kDB = kd.z;
  mat.kAR = ka.x;
  mat.kAG = ka.y;
  mat.kAB = ka.z;
  mat.kSR = ks.x;
  mat.kSG = ks.y;
  mat.kSB = ks.z;
  MaterialCPU matCpu;
  matCpu.albedo = albedoTex;
  matCpu.normal = normalTex;

  uint32_t index;
  m_idxPool.getFreeMemoryData(index);

  // we need to allocate  constant buffer
  matCpu.cbHandle =
      dx12::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(Material));

  m_materialsMagic[index] = MAGIC_NUMBER_COUNTER;
  matCpu.shaderFlags = flags;

  m_materialsCPU[index] = matCpu;
  m_materials[index] = mat;

  MaterialHandle handle{(MAGIC_NUMBER_COUNTER << 16) | (index)};
  ++MAGIC_NUMBER_COUNTER;

  return handle;

  // material handle
}

} // namespace SirEngine