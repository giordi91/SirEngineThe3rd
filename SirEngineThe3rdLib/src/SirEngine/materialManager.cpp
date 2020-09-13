
#include "SirEngine/materialManager.h"

#include <SPIRV-CROSS/spirv_cross.hpp>
#include <cassert>
#include <string>
#include <unordered_map>

#include "SirEngine/PSOManager.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/debugAnnotations.h"
#include "SirEngine/log.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"
#include "SirEngine/skinClusterManager.h"
#include "SirEngine/textureManager.h"
#include "binary/binaryFile.h"
#include "memory/cpu/stringPool.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/vk/vkShaderCompiler.h"
#include "runtimeString.h"

namespace SirEngine {
static const std::string PSO_TYPE_KEY = "type";
static const std::string PSO_TYPE_RASTER = "RASTER";
static const std::string PSO_TYPE_COMPUTE = "COMPUTE";
static const std::string PSO_VS_KEY = "VS";
static const std::string PSO_PS_KEY = "PS";
static const std::string PSO_CS_KEY = "shaderName";
static const std::string PSO_MESH_VERTICES_KEY = "vertices";
static const std::string PSO_MESH_NORMALS_KEY = "normals";
static const std::string PSO_MESH_UVS_KEY = "uvs";
static const std::string PSO_MESH_TANGENTS_KEY = "tangents";

static const std::unordered_map<std::string, MATERIAL_RESOURCE_FLAGS>
    nameToMeshFlag{
        {PSO_MESH_VERTICES_KEY, MATERIAL_RESOURCE_FLAGS::MESH_VERTICES},
        {PSO_MESH_NORMALS_KEY, MATERIAL_RESOURCE_FLAGS::MESH_NORMALS},
        {PSO_MESH_UVS_KEY, MATERIAL_RESOURCE_FLAGS::MESH_UVS},
        {PSO_MESH_TANGENTS_KEY, MATERIAL_RESOURCE_FLAGS::MESH_TANGENTS},
    };

namespace materialKeys {
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
}  // namespace materialKeys

void MaterialManager::bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                                   const MaterialRuntime &materialRuntime) {
  int queueFlagInt = static_cast<int>(queueFlag);
  int currentFlagId = static_cast<int>(log2(queueFlagInt & -queueFlagInt));

  auto handle = materialRuntime.bindingHandle[currentFlagId];
  auto rs = materialRuntime.shaderQueueTypeFlags2[currentFlagId].rs;
  globals::BINDING_TABLE_MANAGER->bindTable(3, handle, rs, false);
}
void MaterialManager::bindMaterial(SHADER_QUEUE_FLAGS queueFlag,
                                   const MaterialHandle handle) {
  const MaterialRuntime &materialRuntime = getMaterialRuntime(handle);
  bindMaterial(queueFlag, materialRuntime);
}

ShaderBind MaterialManager::bindRSandPSO(const uint64_t shaderFlags,
                                         const MaterialHandle handle) const {
  const auto &runtime = getMaterialRuntime(handle);
  // get type flags as int
  constexpr auto mask = static_cast<uint64_t>(~((1ull << 32ull) - 1ull));
  const auto typeFlags = static_cast<uint64_t>(
      (static_cast<uint64_t>(shaderFlags) & mask) >> 32ll);

  for (int i = 0; i < QUEUE_COUNT; ++i) {
    if (runtime.shaderQueueTypeFlags2[i].pso.handle == typeFlags) {
      ShaderBind bind = runtime.shaderQueueTypeFlags2[i];
      globals::ROOT_SIGNATURE_MANAGER->bindGraphicsRS(bind.rs);
      globals::PSO_MANAGER->bindPSO(bind.pso);
      return bind;
    }
  }
  assert(0 && "Could not find requested shader type for PSO /RS bind");
  return {};
}

uint32_t findBindingIndex(const MaterialMetadata *meta,
                          const std::string &bindName) {
  uint32_t count = meta->objectResourceCount;
  for (int i = 0; i < count; ++i) {
    const auto &resource = meta->objectResources[i];
    bool result = strcmp(resource.name, bindName.c_str()) == 0;
    if (result) {
      return i;
    }
  }
  assert(0 && "could not find binding name");
  return 9999;
}

MaterialHandle MaterialManager::loadMaterial(const char *path,
                                             const MeshHandle meshHandle,
                                             const SkinHandle skinHandle) {
  PreliminaryMaterialParse parse = parseMaterial(path, meshHandle, skinHandle);

  uint32_t index;
  MaterialData &materialData =
      m_materialTextureHandles.getFreeMemoryData(index);

  materialData.m_material = parse.mat;

  MaterialRuntime matCpu{};
  matCpu.skinHandle = skinHandle;
  matCpu.meshHandle = meshHandle;

  for (uint32_t i = 0; i < QUEUE_COUNT; ++i) {
    const char *value = parse.shaderQueueTypeFlagsStr[i];
    if (value != nullptr) {
      PSOHandle pso = globals::PSO_MANAGER->getHandleFromName(value);
      RSHandle rs = globals::PSO_MANAGER->getRS(pso);
      matCpu.shaderQueueTypeFlags2[i] = ShaderBind{rs, pso};
    }
  }


  materialData.magicNumber = MAGIC_NUMBER_COUNTER++;

  const std::string name = getFileName(path);
  MaterialHandle handle{(materialData.magicNumber << 16) | (index)};
  m_nameToHandle.insert(name.c_str(), handle);

  // parse material resources

  auto jobj = getJsonObj(path);
  assert(jobj.find("resources") != jobj.end());
  const auto &bindingResources = jobj["resources"];

  // NEW code avoiding the old crap
  for (uint32_t i = 0; i < QUEUE_COUNT; ++i) {
    if (!matCpu.shaderQueueTypeFlags2[i].pso.isHandleValid()) {
      continue;
    }
    ShaderBind bind = matCpu.shaderQueueTypeFlags2[i];

    uint32_t flags =
        parse.isStatic
            ? 0
            : graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED;

    const MaterialMetadata *meta = globals::PSO_MANAGER->getMetadata(bind.pso);

    uint32_t objectsCount = meta->objectResourceCount;
    // zeroing out memory jsut to be safe
    memset(descriptions, 0,
           sizeof(graphics::BindingDescription) * objectsCount);
    for (uint32_t obj = 0; obj < objectsCount; ++obj) {
      const MaterialResource &res = meta->objectResources[obj];
      auto type = res.type;
      GRAPHIC_RESOURCE_TYPE graphicsType;
      switch (type) {
        case MATERIAL_RESOURCE_TYPE::TEXTURE: {
          graphicsType = GRAPHIC_RESOURCE_TYPE::TEXTURE;
          break;
        }
        case MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER: {
          graphicsType = GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER;
          break;
        }
        case MATERIAL_RESOURCE_TYPE::BUFFER: {
          bool readOnly =
              (static_cast<uint32_t>(res.flags) &
               static_cast<uint32_t>(MATERIAL_RESOURCE_FLAGS::READ_ONLY)) > 0;
          if (readOnly) {
            graphicsType = GRAPHIC_RESOURCE_TYPE::READ_BUFFER;
          } else {
            graphicsType = GRAPHIC_RESOURCE_TYPE::READWRITE_BUFFER;
          }
          break;
        }
      }
      descriptions[obj] = {res.binding, graphicsType, res.visibility};
    }

    std::string bindingName = name + "-bindingTable";
    BindingTableHandle bindingTable =
        globals::BINDING_TABLE_MANAGER->allocateBindingTable(
            descriptions, objectsCount,
            parse.isStatic
                ? graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_NONE
                : graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
            bindingName.c_str());
    matCpu.bindingHandle[i] = bindingTable;

    // update material
    for (const auto &subRes : bindingResources) {
      const auto type = subRes["type"].get<std::string>();
      const auto bName = subRes["bindingName"].get<std::string>();
      const auto resPath = subRes["resourcePath"].get<std::string>();

      const std::string resName = getFileName(resPath);
      if (type == "texture") {
        TextureHandle tHandle =
            globals::TEXTURE_MANAGER->loadTexture(resPath.c_str());

        uint32_t bindingIdx = findBindingIndex(meta, bName);

        globals::BINDING_TABLE_MANAGER->bindTexture(
            matCpu.bindingHandle[i], tHandle, bindingIdx, bindingIdx, false);
      } else if (type == "mesh") {
        MeshHandle mHandle =
            globals::MESH_MANAGER->getHandleFromName(resPath.c_str());

        globals::BINDING_TABLE_MANAGER->bindMesh(
            matCpu.bindingHandle[i], mHandle, meta->meshBinding.binding,
            meta->meshBinding.flags);
      }
    }
  }
  materialData.m_materialRuntime = matCpu;

  return handle;
}

inline void freeTextureIfNeeded(const TextureHandle handle) {
  if (handle.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(handle);
  }
}

void MaterialManager::releaseAllMaterialsAndRelatedResources() {
  int count = m_nameToHandle.binCount();
  for (int i = 0; i < count; ++i) {
    if (m_nameToHandle.isBinUsed(i)) {
      MaterialHandle value = m_nameToHandle.getValueAtBin(i);

      // now that we have the handle we can get the data
      assertMagicNumber(value);
      const uint32_t index = getIndexFromHandle(value);
      const MaterialData &data = m_materialTextureHandles.getConstRef(index);

      // NOTE constant buffers don't need to be free singularly since the
      // rendering context will allocate in bulk

      if (data.m_materialRuntime.meshHandle.isHandleValid()) {
        globals::MESH_MANAGER->free(data.m_materialRuntime.meshHandle);
      }
    }
  }
}

void MaterialManager::bindMaterial(const MaterialHandle handle,
                                   SHADER_QUEUE_FLAGS queue) {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const auto &data = m_materialTextureHandles.getConstRef(index);

  const auto currentFlag = static_cast<uint32_t>(queue);
  int currentFlagId = static_cast<int>(log2(currentFlag & -currentFlag));

  ShaderBind bind = data.m_materialRuntime.shaderQueueTypeFlags2[currentFlagId];

  globals::BINDING_TABLE_MANAGER->bindTable(
      3, data.m_materialRuntime.bindingHandle[currentFlagId], bind.rs, false);
}

void MaterialManager::free(const MaterialHandle handle) {
  // TODO properly cleanup the resources
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const auto &data = m_materialTextureHandles.getConstRef(index);

  if (data.name != nullptr) {
    m_nameToHandle.remove(data.name);
  }

  m_materialTextureHandles.free(index);
}


inline uint32_t stringToActualQueueFlag(const std::string &flag) {
  const auto found = materialKeys::STRING_TO_QUEUE_FLAG.find(flag);
  if (found != materialKeys::STRING_TO_QUEUE_FLAG.end()) {
    return static_cast<uint32_t>(found->second);
  }
  assert(0 && "could not map requested queue flag");
  return 0;
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
  float oneFloat = 1.0f;
  float roughnessMult =
      getValueIfInJson(jobj, materialKeys::ROUGHNESS_MULT, oneFloat);
  float metallicMult =
      getValueIfInJson(jobj, materialKeys::METALLIC_MULT, oneFloat);
  bool isStatic = getValueIfInJson(jobj, materialKeys::IS_STATIC_KEY, false);

  const std::string empty;

  Material mat{};
  mat.roughnessMult = roughnessMult;
  mat.metallicMult = metallicMult;

  PreliminaryMaterialParse toReturn;
  toReturn.mat = mat;
  toReturn.isStatic = isStatic;
  parseQueueTypeFlags2(toReturn.shaderQueueTypeFlagsStr, jobj);

  return toReturn;
}

MATERIAL_RESOURCE_FLAGS getMeshFlags(const std::string &name) {
  const auto found = nameToMeshFlag.find(name);
  if (found != nameToMeshFlag.end()) {
    return found->second;
  }
  return MATERIAL_RESOURCE_FLAGS::NONE;
}

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
    default:
      assert(0 && "unsupported visibilty");
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
  for (const auto &image : res.separate_images) {
    auto set = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationDescriptorSet));
    auto binding = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationBinding));

    memory[counter++] = {MATERIAL_RESOURCE_TYPE::TEXTURE,
                         visibility,
                         frameString(image.name.c_str()),
                         MATERIAL_RESOURCE_FLAGS::NONE,
                         set,
                         binding};
  }

  for (const auto &buff : res.storage_buffers) {
    auto set = static_cast<uint16_t>(
        comp.get_decoration(buff.id, spv::DecorationDescriptorSet));
    auto binding = static_cast<uint16_t>(
        comp.get_decoration(buff.id, spv::DecorationBinding));

    spirv_cross::Bitset bufferFlags = comp.get_buffer_block_flags(buff.id);
    bool readonly = bufferFlags.get(spv::DecorationNonWritable);

    MATERIAL_RESOURCE_FLAGS flags = readonly
                                        ? MATERIAL_RESOURCE_FLAGS::READ_ONLY
                                        : MATERIAL_RESOURCE_FLAGS::NONE;
    MATERIAL_RESOURCE_FLAGS meshFlags = type == SHADER_TYPE::VERTEX
                                            ? getMeshFlags(buff.name)
                                            : MATERIAL_RESOURCE_FLAGS::NONE;
    flags = static_cast<MATERIAL_RESOURCE_FLAGS>(
        static_cast<uint32_t>(flags) | static_cast<uint32_t>(meshFlags));

    memory[counter++] = {
        MATERIAL_RESOURCE_TYPE::BUFFER,
        visibility,
        frameString(buff.name.c_str()),
        flags,
        set,
        binding,
    };
  }

  for (const auto &image : res.uniform_buffers) {
    auto set = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationDescriptorSet));
    auto binding = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationBinding));
    memory[counter++] = {MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER,
                         visibility,
                         frameString(image.name.c_str()),
                         MATERIAL_RESOURCE_FLAGS::NONE,
                         set,
                         binding};
  }
  return {memory, nullptr, nullptr, static_cast<uint32_t>(totalCount), 0, 0};
}

MaterialMetadata processComputeMetadata(const nlohmann::json &jobj) {
  assertInJson(jobj, PSO_CS_KEY);

  const std::string &name =
      getValueIfInJson(jobj, PSO_CS_KEY, materialKeys::DEFAULT_STRING);
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

MaterialMeshBinding validateMeshData(const char *name,
                                     const MaterialResource *vsMeta,
                                     int count) {
  // we want to build an array of resources bindings, one per mesh binding type
  // we next want to check they have consecutive values, because it is what the
  // binding system expects
  int slots[4] = {-1, -1, -1, -1};
  uint32_t outFlags = 0;

  int counter = 0;
  for (int i = 0; i < count; ++i) {
    const auto &meta = vsMeta[i];
    auto flags = static_cast<uint32_t>(meta.flags);
    if ((flags &
         static_cast<uint32_t>(MATERIAL_RESOURCE_FLAGS::MESH_VERTICES)) > 0) {
      assert(counter < 4);
      slots[counter++] = meta.binding;
      outFlags |= MESH_ATTRIBUTE_FLAGS::POSITIONS;
    }
    if ((flags & static_cast<uint32_t>(MATERIAL_RESOURCE_FLAGS::MESH_NORMALS)) >
        0) {
      assert(counter < 4);
      slots[counter++] = meta.binding;
      outFlags |= MESH_ATTRIBUTE_FLAGS::NORMALS;
    }
    if ((flags & static_cast<uint32_t>(MATERIAL_RESOURCE_FLAGS::MESH_UVS)) >
        0) {
      assert(counter < 4);
      slots[counter++] = meta.binding;
      outFlags |= MESH_ATTRIBUTE_FLAGS::UV;
    }
    if ((flags &
         static_cast<uint32_t>(MATERIAL_RESOURCE_FLAGS::MESH_TANGENTS)) > 0) {
      assert(counter < 4);
      slots[counter++] = meta.binding;
      outFlags |= MESH_ATTRIBUTE_FLAGS::TANGENTS;
    }
  }
  if (slots[0] == -1) {
    return {-1, MESH_ATTRIBUTE_NONE};
  }

  // lets validate the assumption that they are consecutive
  for (int i = 1; i < counter; ++i) {
    int prev = slots[i - 1];
    int curr = slots[i];
    if ((curr - prev) != 1) {
      SE_CORE_ERROR("Mesh validation failed for {}", name);
      SE_CORE_ERROR(
          "expected consecutive slots but found previous {} and current {}",
          prev, curr);
    }
  }
  return {slots[0], static_cast<MESH_ATTRIBUTE_FLAGS>(outFlags)};
}

MaterialMetadata processRasterMetadata(const char *path,
                                       const nlohmann::json &jobj) {
  assertInJson(jobj, PSO_VS_KEY);

  const std::string &vsName =
      getValueIfInJson(jobj, PSO_VS_KEY, materialKeys::DEFAULT_STRING);
  const std::string &psName =
      getValueIfInJson(jobj, PSO_PS_KEY, materialKeys::DEFAULT_STRING);
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

  MaterialMeshBinding meshBinding = validateMeshData(path, space3, counters[3]);

  MaterialMetadata toReturn{};
  toReturn.meshBinding = meshBinding;
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
    return processRasterMetadata(psoPath, jobj);
  }
  return processComputeMetadata(jobj);
}

MaterialMetadata loadBinaryMetadata(const std::string &psoPath) {
  std::vector<char> binaryData;
  readAllBytes(psoPath, binaryData);

  const auto *const mapper =
      getMapperData<MaterialMappedData>(binaryData.data());
  char *data =
      reinterpret_cast<char *>(binaryData.data() + sizeof(BinaryFileHeader));

  MaterialMetadata outData{};
  uint32_t objectSize = mapper->objectResourceCount * sizeof(MaterialResource);
  uint32_t frameSize = mapper->frameResourceCount * sizeof(MaterialResource);
  uint32_t passSize = mapper->passResourceCount * sizeof(MaterialResource);

  outData.objectResources = static_cast<MaterialResource *>(
      globals::PERSISTENT_ALLOCATOR->allocate(objectSize));
  outData.frameResources =
      static_cast<MaterialResource *>(globals::PERSISTENT_ALLOCATOR->allocate(
          mapper->frameResourceCount * sizeof(MaterialResource)));
  outData.passResources =
      static_cast<MaterialResource *>(globals::PERSISTENT_ALLOCATOR->allocate(
          mapper->passResourceCount * sizeof(MaterialResource)));

  memcpy(outData.objectResources, data + mapper->objectResourceDataOffset,
         objectSize);
  memcpy(outData.frameResources, data + mapper->frameResourceDataOffset,
         frameSize);
  memcpy(outData.passResources, data + mapper->passResourceDataOffset,
         passSize);

  // finally we need to patch the names, names are at the beginning of the
  // buffer one after the other
  uint32_t offset = 0;
  int counter = 0;
  int objectCount = static_cast<int>(mapper->objectResourceCount);
  int upToFrameCount = static_cast<int>(mapper->objectResourceCount +
                                        mapper->frameResourceCount);
  int upToPassCount =
      static_cast<int>(upToFrameCount + mapper->passResourceCount);

  while (offset < mapper->objectResourceDataOffset) {
    // let us find the len of the string
    uint32_t len = static_cast<uint32_t>(strlen(data + offset));
    const char *name = persistentString(data + offset);
    // patch it in the right place
    if (counter < objectCount) {
      outData.objectResources[counter].name = name;
    } else if (counter < upToFrameCount) {
      int idx = counter - objectCount;
      assert(idx >= 0);
      outData.frameResources[idx].name = name;
    } else if (counter < upToPassCount) {
      int idx = counter - upToFrameCount;
      assert(idx >= 0);
      outData.passResources[idx].name = name;
    }
    offset += (len + 1);
    ++counter;
  }
  // patching the count of objects
  outData.objectResourceCount = mapper->objectResourceCount;
  outData.frameResourceCount = mapper->frameResourceCount;
  outData.passResourceCount = mapper->passResourceCount;
  outData.meshBinding =
      MaterialMeshBinding{mapper->meshBinding,
                          static_cast<MESH_ATTRIBUTE_FLAGS>(mapper->meshFlags)};

  return outData;
}

MaterialMetadata loadMetadata(const char *psoPath, GRAPHIC_API api) {
  std::filesystem::path p(psoPath);
  p.replace_extension(".metadata");
  std::string finalP;
  if (api == GRAPHIC_API::VULKAN) {
    finalP = std::string("../data/processed/pso/VK/") + p.filename().string();
  } else {
    finalP = std::string("../data/processed/pso/DX12/") + p.filename().string();
  }
  if (std::filesystem::exists(finalP)) {
    return loadBinaryMetadata(finalP);
  }
  SE_CORE_WARN(
      "Could not find compiled metadata for given PSO: {}\nextracting from "
      "source",
      psoPath);
  return extractMetadata(psoPath);
}

}  // namespace SirEngine
