#include "SirEngine/materialManager.h"
#include "SirEngine/identityManager.h"
#include "SirEngine/fileUtils.h"
#include "nlohmann/json.hpp"
#include <DirectXMath.h>
#include <unordered_map>
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/textureManager.h"
#include "platform/windows/graphics/dx12/constantBufferManager.h"

namespace materialKeys {
static const char *KD = "kd";
static const char *KS = "ks";
static const char *KA = "ka";
static const char *ALBEDO = "albedo";
static const char *NORMAL = "normal";
static const char *FLAGS = "flags";
static const std::unordered_map<std::string, SirEngine::Materials::SHADER_PASS_FLAGS>
    stringToShaderFlag{{"forward", SirEngine::Materials::SHADER_PASS_FLAGS::FORWARD}};

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
namespace Materials {

uint16_t MAGIC_NUMBER_COUNTER = 1;

MaterialHandle loadMaterial(const char *path, uint32_t index,
                            MaterialsMemory &memory) {
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
    IdentityHandle albedoH =
        dx12::IDENTITY_MANAGER->getHandleFromName(albedoName.c_str());
    albedoTex = dx12::TEXTURE_MANAGER->getHandle(albedoH);

  } else {
    // TODO provide white texture as default;
  }
  if (!normalName.empty()) {
    IdentityHandle normalH =
        dx12::IDENTITY_MANAGER->getHandleFromName(normalName.c_str());
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

  // we need to allocate  constant buffer
  matCpu.cbHandle =
      dx12::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(Material));

  (*memory.m_materialsMagic)[index] = MAGIC_NUMBER_COUNTER;
  matCpu.shaderFlags = flags;

  (*memory.m_materialsCPU)[index] = matCpu;
  (*memory.m_materials)[index] = mat;

  MaterialHandle handle{(MAGIC_NUMBER_COUNTER << 16) | (index)};
  ++MAGIC_NUMBER_COUNTER;

  return handle;
}
} // namespace Materials

} // namespace SirEngine
