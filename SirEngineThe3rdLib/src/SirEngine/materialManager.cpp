#include "SirEngine/materialManager.h"
#include "SirEngine/fileUtils.h"
#include "graphics/renderingContext.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"

#if GRAPHICS_API == DX12
#include "platform/windows/graphics/dx12/ConstantBufferManagerDx12.h"
#endif

namespace materialKeys {
static const char *KD = "kd";
static const char *KS = "ks";
static const char *KA = "ka";
static const char *SHINESS = "shiness";
static const char *ALBEDO = "albedo";
static const char *NORMAL = "normal";
static const char *METALLIC = "metallic";
static const char *ROUGHNESS = "roughness";
static const char *AO = "ao";
static const char *SEPARATE_ALPHA = "separateAlpha";
static const char *ROUGHNESS_MULT = "roughnessMult";
static const char *METALLIC_MULT = "metallicMult";
static const char *THICKNESS = "thickness";
static const char *QUEUE = "queue";
static const char *TYPE = "type";
static const std::string DEFAULT_STRING = "";
static const char *RS_KEY = "rs";
static const char *PSO_KEY = "pso";

static const std::unordered_map<std::string, SirEngine::SHADER_QUEUE_FLAGS>
    STRING_TO_QUEUE_FLAG{
        {"forward", SirEngine::SHADER_QUEUE_FLAGS::FORWARD},
        {"deferred", SirEngine::SHADER_QUEUE_FLAGS::DEFERRED},
        {"shadow", SirEngine::SHADER_QUEUE_FLAGS::SHADOW},
    };
static const std::unordered_map<std::string, SirEngine::SHADER_TYPE_FLAGS>
    STRING_TO_TYPE_FLAGS{
        {"pbr", SirEngine::SHADER_TYPE_FLAGS::PBR},
        {"forwardPbr", SirEngine::SHADER_TYPE_FLAGS::FORWARD_PBR},
        {"forwardPhongAlphaCutout",
         SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT},
        {"skin", SirEngine::SHADER_TYPE_FLAGS::SKIN},
        {"hair", SirEngine::SHADER_TYPE_FLAGS::HAIR},
        {"debugLinesColors", SirEngine::SHADER_TYPE_FLAGS::DEBUG_LINES_COLORS},
        {"debugLinesSingleColor",
         SirEngine::SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR},
        {"debugPointsColors",
         SirEngine::SHADER_TYPE_FLAGS::DEBUG_POINTS_COLORS},
        {"debugPointsSingleColor",
         SirEngine::SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR},
        {"debugTrianglesColors",
         SirEngine::SHADER_TYPE_FLAGS::DEBUG_TRIANGLE_COLORS},
        {"debugTrianglesSingleColor",
         SirEngine::SHADER_TYPE_FLAGS::DEBUG_TRIANGLE_SINGLE_COLOR},
    };
static const std::unordered_map<SirEngine::SHADER_TYPE_FLAGS, std::string>
    TYPE_FLAGS_TO_STRING{
        {SirEngine::SHADER_TYPE_FLAGS::PBR, "pbr"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PBR, "forwardPbr"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT,
         "forwardPhongAlphaCutout"},
        {SirEngine::SHADER_TYPE_FLAGS::SKIN, "skin"},
        {SirEngine::SHADER_TYPE_FLAGS::HAIR, "hair"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_LINES_COLORS, "debugLinesColors"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR,
         "debugLinesSingleColor"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_POINTS_COLORS,
         "debugPointsColors"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR,
         "debugPointsSingleColor"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_TRIANGLE_COLORS,
         "debugTrianglesColors"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_TRIANGLE_SINGLE_COLOR,
         "debugTrianglesSingleColor"},
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

uint16_t parseTypeFlags(const nlohmann::json &jobj) {
  if (jobj.find(materialKeys::TYPE) == jobj.end()) {
    assert(0 && "cannot find type flags in material");
    return 0;
  }
  // extract the type flag
  const auto &tJobj = jobj[materialKeys::TYPE];
  const auto stringType = tJobj.get<std::string>();
  const uint16_t typeFlag = stringToActualTypeFlag(stringType);
  return typeFlag;
}

uint32_t parseQueueTypeFlags(const nlohmann::json &jobj) {
  if (jobj.find(materialKeys::QUEUE) == jobj.end()) {
    assert(0 && "cannot find queue flags in material");
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

  const uint32_t typeFlag = parseTypeFlags(jobj);
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
  // bind extra thickness map
  commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.thickness);

  // HARDCODED stencil value might have to think of a nice way to handle this
  commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::SSSSS));
}
void bindForwardPBR(const MaterialRuntime &materialRuntime,
                    ID3D12GraphicsCommandList2 *commandList) {
  const ConstantBufferHandle lightCB = globals::RENDERING_CONTEXT->getLightCB();
  const auto address =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

  commandList->SetGraphicsRootConstantBufferView(1, address);
  commandList->SetGraphicsRootConstantBufferView(
      2, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.metallic);
  commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.roughness);

  TextureHandle skyHandle =
      globals::RENDERING_CONTEXT->getEnviromentMapIrradianceHandle();
  commandList->SetGraphicsRootDescriptorTable(
      7, dx12::TEXTURE_MANAGER->getSRVDx12(skyHandle).gpuHandle);

  TextureHandle skyRadianceHandle =
      globals::RENDERING_CONTEXT->getEnviromentMapRadianceHandle();
  commandList->SetGraphicsRootDescriptorTable(
      8, dx12::TEXTURE_MANAGER->getSRVDx12(skyRadianceHandle).gpuHandle);

  TextureHandle brdfHandle = globals::RENDERING_CONTEXT->getBrdfHandle();
  commandList->SetGraphicsRootDescriptorTable(
      9, dx12::TEXTURE_MANAGER->getSRVDx12(brdfHandle).gpuHandle);
}
void bindForwardPhongAlphaCutout(const MaterialRuntime &materialRuntime,
                                 ID3D12GraphicsCommandList2 *commandList) {
  const ConstantBufferHandle lightCB = globals::RENDERING_CONTEXT->getLightCB();
  const auto address =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

  commandList->SetGraphicsRootConstantBufferView(1, address);
  commandList->SetGraphicsRootConstantBufferView(
      2, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.separateAlpha);
  // HARDCODED stencil value might have to think of a nice way to handle this
  commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::CLEAR));
}
void bindHair(const MaterialRuntime &materialRuntime,
              ID3D12GraphicsCommandList2 *commandList) {
  const ConstantBufferHandle lightCB = globals::RENDERING_CONTEXT->getLightCB();
  const auto address =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

  commandList->SetGraphicsRootConstantBufferView(1, address);
  commandList->SetGraphicsRootConstantBufferView(
      2, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.separateAlpha);
  commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.ao);
  // HARDCODED stencil value might have to think of a nice way to handle this
  commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::CLEAR));
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
  case (SHADER_TYPE_FLAGS::FORWARD_PBR): {
    bindForwardPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT): {
    bindForwardPhongAlphaCutout(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::HAIR): {
    bindHair(materialRuntime, commandList);
    break;
  }
  default: {
    assert(0 && "could not find material type");
  }
  }
}

void MaterialManager::loadTypesInFolder(const char *folder) {
  std::vector<std::string> paths;
  listFilesInFolder(folder, paths, "json");

  for (const auto &p : paths) {
    loadTypeFile(p.c_str());
  }
}

void MaterialManager::bindRSandPSO(const uint32_t shaderFlags,
                                   ID3D12GraphicsCommandList2 *commandList) {
  // get type flags as int
  constexpr auto mask = static_cast<uint32_t>(~((1 << 16) - 1));
  const auto typeFlags = static_cast<uint16_t>((shaderFlags & mask) >> 16);

  const auto found = m_shderTypeToShaderBind.find(typeFlags);
  if (found != m_shderTypeToShaderBind.end()) {
    dx12::ROOT_SIGNATURE_MANAGER->bindGraphicsRS(found->second.rs, commandList);
    dx12::PSO_MANAGER->bindPSO(found->second.pso, commandList);
    return;
  }
  assert(0 && "Could not find requested shader type for PSO /RS bind");
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
  const PSOHandle psoHandle = dx12::PSO_MANAGER->getHandleFromName(psoString);
  const RSHandle rsHandle =
      dx12::ROOT_SIGNATURE_MANAGER->getHandleFromName(rsString);

  std::string name = getFileName(path);

  const uint16_t flags = parseTypeFlags(jObj);
  m_shderTypeToShaderBind[flags] = ShaderBind{rsHandle, psoHandle};
}
MaterialHandle MaterialManager::loadMaterial(const char *path,
                                             MaterialRuntime *materialRuntime, const SkinHandle skinHandle) {
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
  float oneFloat = 1.0f;
  float shininess = getValueIfInJson(jobj, materialKeys::SHINESS, zeroFloat);
  float roughnessMult =
      getValueIfInJson(jobj, materialKeys::ROUGHNESS_MULT, oneFloat);
  float metallicMult =
      getValueIfInJson(jobj, materialKeys::METALLIC_MULT, oneFloat);

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
  const std::string aoName = getValueIfInJson(jobj, materialKeys::AO, empty);

  TextureHandle albedoTex{0};
  TextureHandle normalTex{0};
  TextureHandle metallicTex{0};
  TextureHandle roughnessTex{0};
  TextureHandle thicknessTex{0};
  TextureHandle separateAlphaTex{0};
  TextureHandle aoTex{0};

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
  if (!separateAlphaName.empty()) {
    separateAlphaTex =
        dx12::TEXTURE_MANAGER->loadTexture(separateAlphaName.c_str());
  } else {
    separateAlphaTex = dx12::TEXTURE_MANAGER->getWhiteTexture();
  }
  if (!aoName.empty()) {
    aoTex = dx12::TEXTURE_MANAGER->loadTexture(aoName.c_str());
  } else {
    aoTex = dx12::TEXTURE_MANAGER->getWhiteTexture();
  }

  uint32_t queueTypeFlags = parseQueueTypeFlags(jobj);

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
    texHandles.thicknessSrv = dx12::TEXTURE_MANAGER->getSRVDx12(thicknessTex);
  }
  if (separateAlphaTex.handle != 0) {
    texHandles.separateAlphaSrv =
        dx12::TEXTURE_MANAGER->getSRVDx12(separateAlphaTex);
  }
  if (aoTex.handle != 0) {
    texHandles.aoSrv = dx12::TEXTURE_MANAGER->getSRVDx12(aoTex);
  }
  MaterialRuntime matCpu{};
  matCpu.albedo = texHandles.albedoSrv.gpuHandle;
  matCpu.normal = texHandles.normalSrv.gpuHandle;
  matCpu.metallic = texHandles.metallicSrv.gpuHandle;
  matCpu.roughness = texHandles.roughnessSrv.gpuHandle;
  matCpu.thickness = texHandles.thicknessSrv.gpuHandle;
  matCpu.separateAlpha = texHandles.separateAlphaSrv.gpuHandle;
  matCpu.ao = texHandles.aoSrv.gpuHandle;

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
