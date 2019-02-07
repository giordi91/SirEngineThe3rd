#include "SirEngine/materialManager.h"
#include "SirEngine/IdentityManager.h"
#include "SirEngine/fileUtils.h"
#include "nlohmann/json.hpp"
#include <DirectXMath.h>

namespace materialKeys {
static const char *KD = "kd";
static const char *KS = "ks";
static const char *KA = "ka";
static const char *ALBEDO = "albedo";
static const char *NORMAL = "normal";

} // namespace materialKeys

namespace SirEngine {
void MaterialManager::loadMaterial(const char *path) {
  auto jobj = get_json_obj(path);
  DirectX::XMFLOAT4 zero{0.0f, 0.0f, 0.0f, 0.0f};
  DirectX::XMFLOAT4 kd = getValueIfInJson(jobj, materialKeys::KD, zero);
  DirectX::XMFLOAT4 ka = getValueIfInJson(jobj, materialKeys::KA, zero);
  DirectX::XMFLOAT4 ks = getValueIfInJson(jobj, materialKeys::KS, zero);

  const std::string empty{""};
  const std::string albedoName =
      getValueIfInJson(jobj, materialKeys::ALBEDO, empty);
  const std::string normalName=
      getValueIfInJson(jobj, materialKeys::NORMAL, empty);

  dx12::TextureHandle albedoTex{0};
  dx12::TextureHandle normalTex{0};

  if (!albedoName.empty()) {
    IdentityHandle albedoH =
        dx12::IDENTITY_MANAGER->getHandleFromName(albedoName.c_str());
    albedoTex = dx12::TEXTURE_MANAGER->getHandle(albedoH);
  }
  else
  {
	  //TODO provide white texture as default;
  }
  if (!normalName.empty()) {
    IdentityHandle normalH=
        dx12::IDENTITY_MANAGER->getHandleFromName(albedoName.c_str());
    normalTex= dx12::TEXTURE_MANAGER->getHandle(normalH);
  }
  else
  {
	  //TODO provide white texture as default;
  }

  MaterialCPU matCpu;
  matCpu.mat.kDR = kd.x;
  matCpu.mat.kDG = kd.y;
  matCpu.mat.kDB = kd.z;
  matCpu.mat.kAR = ka.x;
  matCpu.mat.kAG = ka.y;
  matCpu.mat.kAB = ka.z;
  matCpu.mat.kSR = ks.x;
  matCpu.mat.kSG = ks.y;
  matCpu.mat.kSB = ks.z;
  matCpu.albedo = albedoTex;
  matCpu.normal = normalTex;
}

} // namespace SirEngine
