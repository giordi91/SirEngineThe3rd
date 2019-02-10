#include "SirEngine/materialManager.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/identityManager.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/constantBufferManager.h"
#include "platform/windows/graphics/dx12/textureManager.h"
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

MaterialHandle MaterialManager::loadMaterial(const char *path,
                                             uint32_t runtimeIndex,
                                             MaterialRuntime *runtimeMemory) {

  // for materials we do not perform the check whether is loaded or not
  // each object is going to get it s own material copy.
  // if that starts to be an issue we will add extra logic to deal with this.

  assert(fileExists(path));
  const std::string name = getFileName(path);

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

  TextureHandle albedoTex{0};
  TextureHandle normalTex{0};

  if (!albedoName.empty()) {
    albedoTex = dx12::TEXTURE_MANAGER->loadTexture(albedoName.c_str());
  } else {
    // TODO provide white texture as default;
  }
  if (!normalName.empty()) {
    normalTex = dx12::TEXTURE_MANAGER->loadTexture(normalName.c_str());
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

  MaterialRuntime matCpu;
  matCpu.albedo = albedoTex;
  matCpu.normal = normalTex;

  // we need to allocate  constant buffer
  matCpu.cbHandle =
      dx12::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(Material));
  uint32_t index;
  m_idxPool.getFreeMemoryData(index);
  m_materialsMagic[index] = MAGIC_NUMBER_COUNTER;
  matCpu.shaderFlags = flags;

  runtimeMemory[runtimeIndex] = matCpu;
  m_materials[index] = mat;

  MaterialHandle handle{(MAGIC_NUMBER_COUNTER << 16) | (index)};
  ++MAGIC_NUMBER_COUNTER;

  m_nameToHandle[name] = handle;

  return handle;
}

} // namespace SirEngine
