#include "platform/windows/graphics/dx12/dx12MaterialManager.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/renderingContext.h"
#include "SirEngine/skinClusterManager.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/dx12BufferManager.h"
#include "platform/windows/graphics/dx12/dx12MeshManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"

#if GRAPHICS_API == DX12
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
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
static const char *HEIGHT = "height";
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
        {"forwardPhong", SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG},
        {"forwardPhongAlphaCutout",
         SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT},
        {"skin", SirEngine::SHADER_TYPE_FLAGS::SKIN},
        {"hair", SirEngine::SHADER_TYPE_FLAGS::HAIR},
        {"hairSkin", SirEngine::SHADER_TYPE_FLAGS::HAIRSKIN},
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
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT,
         "forwardPhongAlphaCutout"},
        {SirEngine::SHADER_TYPE_FLAGS::SKIN, "skin"},
        {SirEngine::SHADER_TYPE_FLAGS::HAIR, "hair"},
        {SirEngine::SHADER_TYPE_FLAGS::HAIRSKIN, "hairSkin"},
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
        {SirEngine::SHADER_TYPE_FLAGS::SKINCLUSTER, "skinCluster"},
        {SirEngine::SHADER_TYPE_FLAGS::SKINSKINCLUSTER, "skinSkinCluster"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT_SKIN,
         "forwardPhongAlphaCutoutSkin"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PARALLAX, "forwardParallax"},
        {SirEngine::SHADER_TYPE_FLAGS::SHADOW_SKIN_CLUSTER,
         "shadowSkinCluster"},
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

uint16_t parseTypeFlags(const std::string &stringType) {
  const uint16_t typeFlag = stringToActualTypeFlag(stringType);
  return typeFlag;
}

void parseQueueTypeFlags(Dx12MaterialRuntime &matCpu,
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
    const uint32_t typeFlag = parseTypeFlags(stringType);
    flags = typeFlag << 16 | flags;

    matCpu.shaderQueueTypeFlags[currentFlagId] = flags;
  }
}
void bindPBR(const Dx12MaterialRuntime &materialRuntime,
             ID3D12GraphicsCommandList2 *commandList) {

  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

  commandList->SetGraphicsRootConstantBufferView(
      1, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(2, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.metallic);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.roughness);
}

void bindSkinning(const Dx12MaterialRuntime &materialRuntime,
                  ID3D12GraphicsCommandList2 *commandList) {
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

  commandList->SetGraphicsRootConstantBufferView(
      1, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(2, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.metallic);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.roughness);

  // need to bind the skinning data
  const SkinHandle skHandle = materialRuntime.skinHandle;
  const SkinData &data = globals::SKIN_MANAGER->getSkinData(skHandle);
  // now we have both static buffers, influences and weights
  // dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

  // binding skinning data
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.influencesBuffer, 6,
                                                commandList);
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.weightsBuffer, 7,
                                                commandList);
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.matricesBuffer, 8,
                                                commandList);

  dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                               MeshAttributeFlags::ALL, 9);
  // bind mesh data

  // dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(.matricesBuffer, 8,
  //                                              commandList);
}
void bindSkin(const Dx12MaterialRuntime &materialRuntime,
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
void bindSkinSkinning(const Dx12MaterialRuntime &materialRuntime,
                      ID3D12GraphicsCommandList2 *commandList) {
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
  commandList->SetGraphicsRootConstantBufferView(
      1, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(2, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.metallic);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.roughness);
  // bind extra thickness map
  commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.thickness);

  // need to bind the skinning data
  const SkinHandle skHandle = materialRuntime.skinHandle;
  const SkinData &data = globals::SKIN_MANAGER->getSkinData(skHandle);

  // now we have both static buffers, influences and weights
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.influencesBuffer, 7,
                                                commandList);
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.weightsBuffer, 8,
                                                commandList);
  // binding skinning data
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.matricesBuffer, 9,
                                                commandList);
  dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                               MeshAttributeFlags::ALL, 10);

  // HARDCODED stencil value might have to think of a nice way to handle this
  commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::SSSSS));
}
void bindForwardPBR(const Dx12MaterialRuntime &materialRuntime,
                    ID3D12GraphicsCommandList2 *commandList) {
  const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
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
      dx12::RENDERING_CONTEXT->getEnviromentMapIrradianceHandle();
  commandList->SetGraphicsRootDescriptorTable(
      7, dx12::TEXTURE_MANAGER->getSRVDx12(skyHandle).gpuHandle);

  TextureHandle skyRadianceHandle =
      dx12::RENDERING_CONTEXT->getEnviromentMapRadianceHandle();
  commandList->SetGraphicsRootDescriptorTable(
      8, dx12::TEXTURE_MANAGER->getSRVDx12(skyRadianceHandle).gpuHandle);

  TextureHandle brdfHandle = dx12::RENDERING_CONTEXT->getBrdfHandle();
  commandList->SetGraphicsRootDescriptorTable(
      9, dx12::TEXTURE_MANAGER->getSRVDx12(brdfHandle).gpuHandle);
}
void bindForwardPhongAlphaCutout(const Dx12MaterialRuntime &materialRuntime,
                                 ID3D12GraphicsCommandList2 *commandList) {
  const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
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

void bindParallaxPBR(const Dx12MaterialRuntime &materialRuntime,
                     ID3D12GraphicsCommandList2 *commandList) {
  const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
  const auto address =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

  commandList->SetGraphicsRootConstantBufferView(1, address);
  commandList->SetGraphicsRootConstantBufferView(
      2, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.metallic);
  commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.roughness);

  TextureHandle skyHandle =
      dx12::RENDERING_CONTEXT->getEnviromentMapIrradianceHandle();
  commandList->SetGraphicsRootDescriptorTable(
      7, dx12::TEXTURE_MANAGER->getSRVDx12(skyHandle).gpuHandle);

  TextureHandle skyRadianceHandle =
      dx12::RENDERING_CONTEXT->getEnviromentMapRadianceHandle();
  commandList->SetGraphicsRootDescriptorTable(
      8, dx12::TEXTURE_MANAGER->getSRVDx12(skyRadianceHandle).gpuHandle);

  TextureHandle brdfHandle = dx12::RENDERING_CONTEXT->getBrdfHandle();
  commandList->SetGraphicsRootDescriptorTable(
      9, dx12::TEXTURE_MANAGER->getSRVDx12(brdfHandle).gpuHandle);
  commandList->SetGraphicsRootDescriptorTable(10, materialRuntime.heightMap);

  commandList->SetGraphicsRootDescriptorTable(
      11, dx12::TEXTURE_MANAGER
              ->getSRVDx12(globals::DEBUG_FRAME_DATA->directionalShadow)
              .gpuHandle);
  dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                               MeshAttributeFlags::ALL, 12);
}

void bindForwardPhongAlphaCutoutSkin(const Dx12MaterialRuntime &materialRuntime,
                                     ID3D12GraphicsCommandList2 *commandList) {
  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);
  const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
  const auto address =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

  commandList->SetGraphicsRootConstantBufferView(1, address);
  commandList->SetGraphicsRootConstantBufferView(
      2, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.separateAlpha);

  // need to bind the skinning data
  const SkinHandle skHandle = materialRuntime.skinHandle;
  const SkinData &data = globals::SKIN_MANAGER->getSkinData(skHandle);
  // now we have both static buffers, influences and weights
  // dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

  // frame, binding material should not worry about upload
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.influencesBuffer, 6,
                                                commandList);
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.weightsBuffer, 7,
                                                commandList);
  // binding skinning data
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.matricesBuffer, 8,
                                                commandList);
  dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                               MeshAttributeFlags::ALL, 9);

  // HARDCODED stencil value might have to think of a nice way to handle this
  commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::CLEAR));
}
void bindHair(const Dx12MaterialRuntime &materialRuntime,
              ID3D12GraphicsCommandList2 *commandList) {

  dx12::RENDERING_CONTEXT->bindCameraBuffer(0);

  const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
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
void bindHairSkin(const Dx12MaterialRuntime &materialRuntime,
                  ID3D12GraphicsCommandList2 *commandList) {
  const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
  const auto address =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

  commandList->SetGraphicsRootConstantBufferView(1, address);
  commandList->SetGraphicsRootConstantBufferView(
      2, materialRuntime.cbVirtualAddress);
  commandList->SetGraphicsRootDescriptorTable(3, materialRuntime.albedo);
  commandList->SetGraphicsRootDescriptorTable(4, materialRuntime.normal);
  commandList->SetGraphicsRootDescriptorTable(5, materialRuntime.separateAlpha);
  commandList->SetGraphicsRootDescriptorTable(6, materialRuntime.ao);

  // need to bind the skinning data
  const SkinHandle skHandle = materialRuntime.skinHandle;
  const SkinData &data = globals::SKIN_MANAGER->getSkinData(skHandle);
  // now we have both static buffers, influences and weights
  // dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

  // frame, binding material should not worry about upload
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.influencesBuffer, 7,
                                                commandList);
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.weightsBuffer, 8,
                                                commandList);

  // binding skinning matrices
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.matricesBuffer, 9,
                                                commandList);

  dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                               MeshAttributeFlags::ALL, 10);

  // HARDCODED stencil value might have to think of a nice way to handle this
  commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::CLEAR));
}

void bindShadowSkin(const Dx12MaterialRuntime &materialRuntime,
                    ID3D12GraphicsCommandList2 *commandList) {
  const ConstantBufferHandle lightCB = dx12::RENDERING_CONTEXT->getLightCB();
  const auto address =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(lightCB);

  commandList->SetGraphicsRootConstantBufferView(0, address);
  // need to bind the skinning data
  const SkinHandle skHandle = materialRuntime.skinHandle;
  const SkinData &data = globals::SKIN_MANAGER->getSkinData(skHandle);
  // now we have both static buffers, influences and weights
  // dx12::BUFFER_MANAGER->bindBufferAsSRVDescriptorTable(data.influencesBuffer,6,commandList);

  // frame, binding material should not worry about upload
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.influencesBuffer, 1,
                                                commandList);
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.weightsBuffer, 2,
                                                commandList);
  // binding skinning data
  dx12::BUFFER_MANAGER->bindBufferAsSRVGraphics(data.matricesBuffer, 3,
                                                commandList);

  dx12::MESH_MANAGER->bindMesh(materialRuntime.meshHandle, commandList,
                               MeshAttributeFlags::POSITIONS, 4);
  // TODO HARDCODED stencil value might have to think of a nice way to handle
  // this
  commandList->OMSetStencilRef(static_cast<uint32_t>(STENCIL_REF::CLEAR));
}

void Dx12MaterialManager::bindMaterial(
    SHADER_QUEUE_FLAGS queueFlag, const Dx12MaterialRuntime &materialRuntime,
    ID3D12GraphicsCommandList2 *commandList) {
  int queueFlagInt = static_cast<int>(queueFlag);
  int currentFlagId = static_cast<int>(log2(queueFlagInt & -queueFlagInt));
  const SHADER_TYPE_FLAGS type =
      getTypeFlags(materialRuntime.shaderQueueTypeFlags[currentFlagId]);
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
  case (SHADER_TYPE_FLAGS::SKINCLUSTER): {
    bindSkinning(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SKINSKINCLUSTER): {
    bindSkinSkinning(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT_SKIN): {
    bindForwardPhongAlphaCutoutSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::HAIRSKIN): {
    bindHairSkin(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::FORWARD_PARALLAX): {
    bindParallaxPBR(materialRuntime, commandList);
    break;
  }
  case (SHADER_TYPE_FLAGS::SHADOW_SKIN_CLUSTER): {
    bindShadowSkin(materialRuntime, commandList);
    break;
  }
  default: {
    assert(0 && "could not find material type");
  }
  }
}

void Dx12MaterialManager::bindMaterial(
    SHADER_QUEUE_FLAGS queueFlag, const MaterialHandle handle,
    ID3D12GraphicsCommandList2 *commandList) {

  const Dx12MaterialRuntime &materialRuntime = getMaterialRuntime(handle);
  bindMaterial(queueFlag, materialRuntime, commandList);
}

void Dx12MaterialManager::loadTypesInFolder(const char *folder) {
  std::vector<std::string> paths;
  listFilesInFolder(folder, paths, "json");

  for (const auto &p : paths) {
    loadTypeFile(p.c_str());
  }
}

void Dx12MaterialManager::bindRSandPSO(
    const uint32_t shaderFlags, ID3D12GraphicsCommandList2 *commandList) {
  // get type flags as int
  constexpr auto mask = static_cast<uint32_t>(~((1 << 16) - 1));
  const auto typeFlags = static_cast<uint16_t>((shaderFlags & mask) >> 16);

  ShaderBind bind;
  bool found = m_shderTypeToShaderBind.get(typeFlags, bind);
  if (found) {
    dx12::ROOT_SIGNATURE_MANAGER->bindGraphicsRS(bind.rs, commandList);
    dx12::PSO_MANAGER->bindPSO(bind.pso, commandList);
    return;
  }
  assert(0 && "Could not find requested shader type for PSO /RS bind");
}

void Dx12MaterialManager::loadTypeFile(const char *path) {
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
      dx12::PSO_MANAGER->getHandleFromName(psoString.c_str());
  const RSHandle rsHandle =
      dx12::ROOT_SIGNATURE_MANAGER->getHandleFromName(rsString.c_str());

  std::string name = getFileName(path);

  const std::string type = jObj[materialKeys::TYPE].get<std::string>();
  const uint16_t flags = parseTypeFlags(type);
  m_shderTypeToShaderBind.insert(flags, ShaderBind{rsHandle, psoHandle});
}
MaterialHandle Dx12MaterialManager::loadMaterial(const char *path,
                                                 const MeshHandle meshHandle,
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
  if (!heightName.empty()) {
    heightTex = dx12::TEXTURE_MANAGER->loadTexture(heightName.c_str());
  } else {
    heightTex = dx12::TEXTURE_MANAGER->getWhiteTexture();
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

  uint32_t index;
  MaterialData &materialData =
      m_materialTextureHandles.getFreeMemoryData(index);

  if (albedoTex.handle != 0) {
    materialData.albedoSrv = dx12::TEXTURE_MANAGER->getSRVDx12(albedoTex);
  }
  if (normalTex.handle != 0) {
    materialData.normalSrv = dx12::TEXTURE_MANAGER->getSRVDx12(normalTex);
  }
  if (metallicTex.handle != 0) {
    materialData.metallicSrv = dx12::TEXTURE_MANAGER->getSRVDx12(metallicTex);
  }
  if (roughnessTex.handle != 0) {
    materialData.roughnessSrv = dx12::TEXTURE_MANAGER->getSRVDx12(roughnessTex);
  }
  if (thicknessTex.handle != 0) {
    materialData.thicknessSrv = dx12::TEXTURE_MANAGER->getSRVDx12(thicknessTex);
  }
  if (separateAlphaTex.handle != 0) {
    materialData.separateAlphaSrv =
        dx12::TEXTURE_MANAGER->getSRVDx12(separateAlphaTex);
  }
  if (aoTex.handle != 0) {
    materialData.aoSrv = dx12::TEXTURE_MANAGER->getSRVDx12(aoTex);
  }
  if (heightTex.handle != 0) {
    materialData.heightSrv = dx12::TEXTURE_MANAGER->getSRVDx12(heightTex);
  }
  Dx12MaterialRuntime matCpu{};
  matCpu.albedo = materialData.albedoSrv.gpuHandle;
  matCpu.normal = materialData.normalSrv.gpuHandle;
  matCpu.metallic = materialData.metallicSrv.gpuHandle;
  matCpu.roughness = materialData.roughnessSrv.gpuHandle;
  matCpu.thickness = materialData.thicknessSrv.gpuHandle;
  matCpu.separateAlpha = materialData.separateAlphaSrv.gpuHandle;
  matCpu.ao = materialData.aoSrv.gpuHandle;
  matCpu.skinHandle = skinHandle;
  matCpu.heightMap = materialData.heightSrv.gpuHandle;
  matCpu.meshHandle = meshHandle;
  parseQueueTypeFlags(matCpu, jobj);

  // we need to allocate  constant buffer
  // TODO should this be static constant buffer? investigate
  texHandles.cbHandle =
      globals::CONSTANT_BUFFER_MANAGER->allocateDynamic(sizeof(Material), &mat);

  matCpu.cbVirtualAddress =
      dx12::CONSTANT_BUFFER_MANAGER->getVirtualAddress(texHandles.cbHandle);

  materialData.handles = texHandles;
  materialData.magicNumber = MAGIC_NUMBER_COUNTER++;
  materialData.m_materialRuntime = matCpu;

  MaterialHandle handle{(materialData.magicNumber << 16) | (index)};

  m_nameToHandle.insert(name.c_str(), handle);

  return handle;
}

const char *
Dx12MaterialManager::getStringFromShaderTypeFlag(const SHADER_TYPE_FLAGS type) {
  const auto found = materialKeys::TYPE_FLAGS_TO_STRING.find(type);
  if (found != materialKeys::TYPE_FLAGS_TO_STRING.end()) {
    return found->second.c_str();
  }

  assert(0 && "Could not find flag");
  const auto unknown =
      materialKeys::TYPE_FLAGS_TO_STRING.find(SHADER_TYPE_FLAGS::UNKNOWN);
  return unknown->second.c_str();
}

} // namespace SirEngine
