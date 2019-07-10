#include "SirEngine/materialManager.h"
#include "SirEngine/fileUtils.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"

#if GRAPHICS_API == DX12
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#endif

namespace materialKeys {
static const char *KD = "kd";
static const char *KS = "ks";
static const char *KA = "ka";
static const char *PBR = "pbr";
static const char *SHINESS = "shiness";
static const char *ALBEDO = "albedo";
static const char *NORMAL = "normal";
static const char *METALLIC = "metallic";
static const char *ROUGHNESS = "roughness";
static const char *THICKNESS = "thickness";
static const char *QUEUE = "queue";
static const char *TYPE = "type";

static const std::unordered_map<std::string, SirEngine::SHADER_QUEUE_FLAGS>
    STRING_TO_QUEUE_FLAG{
        {"forward", SirEngine::SHADER_QUEUE_FLAGS::FORWARD},
        {"deferred", SirEngine::SHADER_QUEUE_FLAGS::DEFERRED},
        {"shadow", SirEngine::SHADER_QUEUE_FLAGS::SHADOW},
    };
static const std::unordered_map<std::string, SirEngine::SHADER_TYPE_FLAGS>
    STRING_TO_TYPE_FLAGS{
        {"pbr", SirEngine::SHADER_TYPE_FLAGS::PBR},
        {"skin", SirEngine::SHADER_TYPE_FLAGS::SKIN},
    };
static const std::unordered_map<SirEngine::SHADER_TYPE_FLAGS, std::string>
    TYPE_FLAGS_TO_STRING{
        {SirEngine::SHADER_TYPE_FLAGS::PBR, "pbr"},
        {SirEngine::SHADER_TYPE_FLAGS::SKIN, "skin"},
    };

} // namespace materialKeys

namespace SirEngine {
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

uint32_t parseQueueTypeFlags(const nlohmann::json &jobj) {
  if (jobj.find(materialKeys::QUEUE) == jobj.end()) {
    assert(0 && "cannot find queue flags in material");
    return 0;
  }
  if (jobj.find(materialKeys::TYPE) == jobj.end()) {
    assert(0 && "cannot find type flags in material");
    return 0;
  }

  // extract the queue flags, that flag is a bit field for an
  // uint16, which is merged with the material type which
  // is a normal increasing uint16
  const auto &qjobj = jobj[materialKeys::QUEUE];
  uint32_t flags = 0;
  for (const auto &flag : qjobj) {
    const auto stringFlag = flag.get<std::string>();
    const uint32_t currentFlag = stringToActualQueueFlag(stringFlag);
    flags |= currentFlag;
  }

  // extract the type flag
  const auto &tjobj = jobj[materialKeys::TYPE];
  const auto stringType = tjobj.get<std::string>();
  const uint16_t typeFlag = stringToActualTypeFlag(stringType);

  flags = typeFlag << 16 | flags;
  return flags;
}
void bindPBR(const MaterialRuntime &materialRuntime,
             ID3D12GraphicsCommandList2 *commandList) {
  commandList->SetGraphicsRootConstantBufferView(
      1, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(2, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.metallic);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.roughness);
}
void bindSkin(const MaterialRuntime &materialRuntime,
              ID3D12GraphicsCommandList2 *commandList) {
  commandList->SetGraphicsRootConstantBufferView(
      1, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(2, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.metallic);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.roughness);
  // bind extra thinkess map
}

void MaterialManager::bindMaterial(const MaterialRuntime &materialRuntime,
                                   ID3D12GraphicsCommandList2 *commandList) {
  const SHADER_TYPE_FLAGS type =
      getTypeFlags(materialRuntime.shaderQueueTypeFlags);
  switch (type) {
  case (SHADER_TYPE_FLAGS::PBR): {
    bindPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SKIN): {
    bindSkin(materialRuntime, commandList);
    break;
  }
  default: {
    assert(0 && "could not find material type");
  }
  }
}

MaterialHandle MaterialManager::loadMaterial(const char *path,
                                             MaterialRuntime *materialRuntime) {

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
  float zeroFloat = 0.0f;
  float shininess = getValueIfInJson(jobj, materialKeys::SHINESS, zeroFloat);

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

  TextureHandle albedoTex{0};
  TextureHandle normalTex{0};
  TextureHandle metallicTex{0};
  TextureHandle roughnessTex{0};
  TextureHandle thicknessTex{0};

  if (!albedoName.empty()) {
    albedoTex = dx12::TEXTURE_MANAGER->loadTexture(albedoName.c_str());
  } else {
    albedoTex = dx12::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!normalName.empty()) {
    normalTex = dx12::TEXTURE_MANAGER->loadTexture(normalName.c_str());
  } else {
    normalTex = dx12::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!metallicName.empty()) {
    metallicTex = dx12::TEXTURE_MANAGER->loadTexture(metallicName.c_str());
  } else {
    metallicTex = dx12::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!roughnessName.empty()) {
    roughnessTex = dx12::TEXTURE_MANAGER->loadTexture(roughnessName.c_str());
  } else {
    roughnessTex = dx12::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!thicknessName.empty()) {
    thicknessTex = dx12::TEXTURE_MANAGER->loadTexture(thicknessName.c_str());
  } else {
    thicknessTex = dx12::TEXTURE_MANAGER->getWhiteTexture();
  }

  uint32_t queueTypeFlags = parseQueueTypeFlags(jobj);

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
  mat.shiness = shininess;

  MaterialDataHandles texHandles{};
  texHandles.albedo = albedoTex;
  texHandles.normal = normalTex;
  texHandles.metallic = metallicTex;
  texHandles.roughness = roughnessTex;
  texHandles.thickness = thicknessTex;

  if (albedoTex.handle != 0) {
    texHandles.albedoSrv = dx12::TEXTURE_MANAGER->getSRVDx12(albedoTex);
  }
  if (normalTex.handle != 0) {
    texHandles.normalSrv = dx12::TEXTURE_MANAGER->getSRVDx12(normalTex);
  }
  if (metallicTex.handle != 0) {
    texHandles.metallicSrv = dx12::TEXTURE_MANAGER->getSRVDx12(metallicTex);
  }
  if (roughnessTex.handle != 0) {
    texHandles.roughnessSrv = dx12::TEXTURE_MANAGER->getSRVDx12(roughnessTex);
  }
  if (thicknessTex.handle != 0) {
    texHandles.thicknessSrv= dx12::TEXTURE_MANAGER->getSRVDx12(thicknessTex);
  }
  MaterialRuntime matCpu{};
  matCpu.albedo = texHandles.albedoSrv.gpuHandle;
  matCpu.normal = texHandles.normalSrv.gpuHandle;
  matCpu.metallic = texHandles.metallicSrv.gpuHandle;
  matCpu.roughness = texHandles.roughnessSrv.gpuHandle;
  matCpu.thickness= texHandles.thicknessSrv.gpuHandle;

  // we need to allocate  constant buffer
  // TODO should this be static constant buffer? investigate
  texHandles.cbHandle =
      globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(Material), &mat);
  uint32_t index;
  m_idxPool.getFreeMemoryData(index);
  m_materialsMagic[index] = static_cast<uint16_t>(MAGIC_NUMBER_COUNTER);
  matCpu.shaderQueueTypeFlags = queueTypeFlags;

  matCpu.cbVirtualAddress =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(texHandles.cbHandle);

  (*materialRuntime) = matCpu;
  m_materials[index] = mat;
  m_materialTextureHandles[index] = texHandles;

  MaterialHandle handle{(MAGIC_NUMBER_COUNTER << 16) | (index)};
  ++MAGIC_NUMBER_COUNTER;

  m_nameToHandle[name] = handle;

  return handle;
}

const std::string &
MaterialManager::getStringFromShaderTypeFlag(const SHADER_TYPE_FLAGS type) {
  const auto found = materialKeys::TYPE_FLAGS_TO_STRING.find(type);
  if (found != materialKeys::TYPE_FLAGS_TO_STRING.end()) {
    return found->second;
  }

  assert(0 && "Could not find flag");
  const auto unknown =
      materialKeys::TYPE_FLAGS_TO_STRING.find(SHADER_TYPE_FLAGS::UNKNOWN);
  return unknown->second;
}

} // namespace SirEngine
