
#include "SirEngine/materialManager.h"

#include <assert.h>

#include <string>
#include <unordered_map>

#include "SirEngine/engineMath.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/materialMetadata.h"
#include "SirEngine/io/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"
#include "SirEngine/runtimeString.h"
#include "SirEngine/textureManager.h"
#include "constantBufferManager.h"
#include "engineConfig.h"
#include "nlohmann/json.hpp"

namespace SirEngine {

namespace materialKeys {
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
static const std::unordered_map<std::string, SirEngine::NUMERICAL_DATA_TYPE>
    STRING_TO_NUMERICAL_TYPE{
        {"undefined", NUMERICAL_DATA_TYPE::UNDEFINED},
        {"float", NUMERICAL_DATA_TYPE::FLOAT},
        {"int", NUMERICAL_DATA_TYPE::INT},
        {"boolean", NUMERICAL_DATA_TYPE::BOOLEAN},
        {"vec2", NUMERICAL_DATA_TYPE::VEC2},
        {"vec3", NUMERICAL_DATA_TYPE::VEC3},
        {"vec4", NUMERICAL_DATA_TYPE::VEC4},
        {"mat3", NUMERICAL_DATA_TYPE::MAT3},
        {"mat4", NUMERICAL_DATA_TYPE::MAT4},
        {"int16", NUMERICAL_DATA_TYPE::INT16},
        {"float16", NUMERICAL_DATA_TYPE::FLOAT16},
    };
}  // namespace materialKeys

static const std::unordered_map<NUMERICAL_DATA_TYPE, uint32_t>
    NUMERICAL_TYPE_TO_SIZE{
        {NUMERICAL_DATA_TYPE::UNDEFINED, 0}, {NUMERICAL_DATA_TYPE::FLOAT, 4},
        {NUMERICAL_DATA_TYPE::INT, 4},       {NUMERICAL_DATA_TYPE::BOOLEAN, 1},
        {NUMERICAL_DATA_TYPE::VEC2, 8},      {NUMERICAL_DATA_TYPE::VEC3, 12},
        {NUMERICAL_DATA_TYPE::VEC4, 16},     {NUMERICAL_DATA_TYPE::MAT3, 36},
        {NUMERICAL_DATA_TYPE::MAT4, 64},     {NUMERICAL_DATA_TYPE::INT16, 2},
        {NUMERICAL_DATA_TYPE::FLOAT16, 2},
    };

ShaderBind MaterialManager::bindRSandPSO(const uint64_t shaderFlags,
                                         const MaterialHandle handle) const {
  const auto &runtime = getMaterialData(handle);
  // get type flags as int
  constexpr auto mask = static_cast<uint64_t>(~((1ull << 32ull) - 1ull));
  const auto typeFlags = static_cast<uint64_t>(
      (static_cast<uint64_t>(shaderFlags) & mask) >> 32ll);

  for (int i = 0; i < QUEUE_COUNT; ++i) {
    if (runtime.shaderBindPerQueue[i].pso.handle == typeFlags) {
      ShaderBind bind = runtime.shaderBindPerQueue[i];
      globals::ROOT_SIGNATURE_MANAGER->bindGraphicsRS(bind.rs);
      globals::PSO_MANAGER->bindPSO(bind.pso);
      return bind;
    }
  }
  assert(0 && "Could not find requested shader type for PSO /RS bind");
  return {};
}

int findBindingIndex(const graphics::MaterialMetadata *meta,
                     const std::string &bindName) {
  uint32_t count = meta->objectResourceCount;
  uint32_t pushOffset = meta->hasObjectPushConstant() ? 1 : 0;
  for (uint32_t i = pushOffset; i < count; ++i) {
    const auto &resource = meta->objectResources[i];
    bool result = strcmp(resource.name, bindName.c_str()) == 0;
    if (result) {
      if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::VULKAN) {
        return static_cast<int>(resource.binding);
      }
      return static_cast<int>(i) - pushOffset;
    }
  }
  assert(0 && "could not find binding name");
  return -1;
}

int MaterialManager::buildBindingTableDefinitionFromMetadta(
    const graphics::MaterialMetadata *meta) {
  // push constants needs to be skipped because are part of the pipelien state
  // object not of the binding table, not sure about DX12, need to investigate
  int pushOffset = meta->hasObjectPushConstant() ? 1 : 0;
  uint32_t objectsCount = meta->objectResourceCount - pushOffset;
  // zeroing out memory just: to be safe
  memset(m_descriptions, 0,
         sizeof(graphics::BindingDescription) * objectsCount);
  for (uint32_t obj = 0; obj < objectsCount; ++obj) {
    const graphics::MaterialResource &res =
        meta->objectResources[obj + pushOffset];
    auto type = res.type;
    GRAPHIC_RESOURCE_TYPE graphicsType = GRAPHIC_RESOURCE_TYPE::NONE;
    switch (type) {
      case graphics::MATERIAL_RESOURCE_TYPE::TEXTURE: {
        graphicsType = GRAPHIC_RESOURCE_TYPE::TEXTURE;
        break;
      }
      case graphics::MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER: {
        graphicsType = GRAPHIC_RESOURCE_TYPE::CONSTANT_BUFFER;
        break;
      }
      case graphics::MATERIAL_RESOURCE_TYPE::BUFFER: {
        bool readOnly = (static_cast<uint32_t>(res.flags) &
                         static_cast<uint32_t>(
                             graphics::MATERIAL_RESOURCE_FLAGS::READ_ONLY)) > 0;
        if (readOnly) {
          graphicsType = GRAPHIC_RESOURCE_TYPE::READ_BUFFER;
        } else {
          graphicsType = GRAPHIC_RESOURCE_TYPE::READWRITE_BUFFER;
        }
        break;
      }
    }
    m_descriptions[obj] = {res.binding, graphicsType, res.visibility};
  }
  return objectsCount;
}

int findIndexInObjectMaterialBinding(const char *const bindingName,
                                     const graphics::MaterialMetadata *meta) {
  uint32_t count = meta->objectResourceCount;
  for (uint32_t i = 0; i < count; ++i) {
    if (strcmp(bindingName, meta->objectResources[i].name) == 0) {
      return i;
    }
  }
  return -1;
}

int findIndexOfConstantBufferMember(
    const char *name, const graphics::MaterialResource &objResource) {
  uint32_t count = objResource.extension.uniform.membersCount;
  for (uint32_t i = 0; i < count; ++i) {
    const graphics::MaterialMetadataStructMember &member =
        objResource.extension.uniform.members[i];
    if (strcmp(member.name, name) == 0) {
      return i;
    }
  }
  return -1;
}

MaterialHandle MaterialManager::loadMaterial(const char *path) {
  PreliminaryMaterialParse parse = parseMaterial(path);

  uint32_t index;
  MaterialData &materialData =
      m_materialTextureHandles.getFreeMemoryData(index);

  for (uint32_t i = 0; i < QUEUE_COUNT; ++i) {
    const char *value = parse.shaderQueueTypeFlagsStr[i];
    if (value != nullptr) {
      PSOHandle pso = globals::PSO_MANAGER->getHandleFromName(value);
      RSHandle rs = globals::PSO_MANAGER->getRS(pso);
      materialData.shaderBindPerQueue[i] = ShaderBind{rs, pso};
    }
  }

  materialData.magicNumber = MAGIC_NUMBER_COUNTER++;

  const std::string name = getFileName(path);
  MaterialHandle handle{(materialData.magicNumber << 16) | (index)};
  m_nameToHandle.insert(name.c_str(), handle);

  for (uint32_t i = 0; i < QUEUE_COUNT; ++i) {
    if (!materialData.shaderBindPerQueue[i].pso.isHandleValid()) {
      continue;
    }
    ShaderBind bind = materialData.shaderBindPerQueue[i];

    const graphics::MaterialMetadata *meta =
        globals::PSO_MANAGER->getMetadata(bind.pso);

    int bindingTableCount = buildBindingTableDefinitionFromMetadta(meta);

    std::string bindingName = name + "-bindingTable";
    assert(parse.isStatic &&
           "we do not support yet dynamic materials from assets");

    BindingTableHandle bindingTable =
        globals::BINDING_TABLE_MANAGER->allocateBindingTable(
            m_descriptions, bindingTableCount,
            parse.isStatic
                ? graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_NONE
                : graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
            bindingName.c_str());
    materialData.bindingHandle[i] = bindingTable;

    bool isDx12 = globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12;
    // update material
    for (uint32_t res = 0; res < parse.sourceBindingsCount; ++res) {
      MaterialSourceBinding &matBinding = parse.sourceBindings[res];
      const std::string resName = getFileName(matBinding.resourcePath);
      if (strcmp(matBinding.type, "texture") == 0) {
        TextureHandle tHandle =
            globals::TEXTURE_MANAGER->loadTexture(matBinding.resourcePath);
        matBinding.resourceHandle = tHandle.handle;

        uint32_t bindingIdx = findBindingIndex(meta, matBinding.bindingName);

        globals::BINDING_TABLE_MANAGER->bindTexture(
            materialData.bindingHandle[i], tHandle, bindingIdx, bindingIdx,
            false);
      } else if (strcmp(matBinding.type, "mesh") == 0) {
        MeshHandle mHandle =
            globals::MESH_MANAGER->getHandleFromName(resName.c_str());
        matBinding.resourceHandle = mHandle.handle;

        uint32_t pushOffset = meta->hasObjectPushConstant() ? 1 : 0;
        globals::BINDING_TABLE_MANAGER->bindMesh(
            materialData.bindingHandle[i], mHandle,
            // Push/Root constant must appear at the end, this gives us
            // different bindings, but also if the push constant appears it means
            // we need to offset by one the binding for dx12, since the root
            // constant does not goes in descriptor set. This happen the same for
            // when we find the binding index of something in the constant buffer
            // below. Be mindful of it, is not as transparent as I would like
            // but for now it will do
            isDx12 ? meta->meshBinding.dxBinding - pushOffset
                   : meta->meshBinding.vkBinding,
            meta->meshBinding.flags);
      } else if (strcmp(matBinding.type, "constantBuffer") == 0) {
        // allocate the necessary constant buffer
        uint32_t arrayIndex =
            findIndexInObjectMaterialBinding(matBinding.bindingName, meta);
        const auto &objResource = meta->objectResources[arrayIndex];
        assert(objResource.type ==
               graphics::MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER);
        uint32_t bufferSize = objResource.extension.uniform.structSize;
        char *loadedData =
            static_cast<char *>(globals::FRAME_ALLOCATOR->allocate(bufferSize));

        // we have the cpu data we need to load the data in there
        int subCount = matBinding.subBindingCount;
        for (int s = 0; s < subCount; ++s) {
          const MaterialSourceSubBinding &sub = matBinding.subBinding[s];
          // need to find the offset in the shader
          int constantIdx =
              findIndexOfConstantBufferMember(sub.name, objResource);
          assert(constantIdx != -1);
          assert(objResource.extension.uniform.members[constantIdx].size ==
                 sub.sizeInByte);

          int offset =
              objResource.extension.uniform.members[constantIdx].offset;
          memcpy(loadedData + offset, sub.value, sub.sizeInByte);
        }

        ConstantBufferHandle cHandle =
            globals::CONSTANT_BUFFER_MANAGER->allocate(
                bufferSize,
                ConstantBufferManager::CONSTANT_BUFFER_FLAG_BITS::NONE,
                loadedData);
        matBinding.resourceHandle = cHandle.handle;

        uint32_t bindingIdx = findBindingIndex(meta, matBinding.bindingName);
        globals::BINDING_TABLE_MANAGER->bindConstantBuffer(
            materialData.bindingHandle[i], cHandle, bindingIdx, bindingIdx);
      }
    }
  }
  materialData.materialBindingCount = parse.sourceBindingsCount;
  materialData.materialBinding = parse.sourceBindings;

  return handle;
}

PSOHandle MaterialManager::getmaterialPSO(const MaterialHandle handle,
                                          SHADER_QUEUE_FLAGS queue) const {
  assertMagicNumber(handle);
  uint32_t index = getIndexFromHandle(handle);
  const auto &data = m_materialTextureHandles.getConstRef(index);
  uint32_t queueIdx = getFirstBitSet(static_cast<uint32_t>(queue));
  assert(queueIdx < QUEUE_COUNT);
  return data.shaderBindPerQueue[queueIdx].pso;
}

inline void freeTextureIfNeeded(const TextureHandle handle) {
  if (handle.isHandleValid()) {
    globals::TEXTURE_MANAGER->free(handle);
  }
}

void MaterialManager::cleanup() {
  int count = m_nameToHandle.binCount();
  for (int i = 0; i < count; ++i) {
    if (m_nameToHandle.isBinUsed(i)) {
      MaterialHandle value = m_nameToHandle.getValueAtBin(i);

      // now that we have the handle we can get the data
      assertMagicNumber(value);
      const uint32_t index = getIndexFromHandle(value);
      const MaterialData &data = m_materialTextureHandles.getConstRef(index);

      for (uint32_t q = 0; q < QUEUE_COUNT; ++q) {
        if (data.bindingHandle[q].isHandleValid()) {
          globals::BINDING_TABLE_MANAGER->free(data.bindingHandle[q]);
        }
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
  int currentFlagId = getFirstBitSet(currentFlag);

  ShaderBind bind = data.shaderBindPerQueue[currentFlagId];

  globals::BINDING_TABLE_MANAGER->bindTable(
      3, data.bindingHandle[currentFlagId], bind.rs, false);
}

void MaterialManager::free(const MaterialHandle handle) {
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

static void parseQueueTypeFlags(const char **outFlags,
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
    int currentFlagId = getFirstBitSet(currentFlag);

    const auto stringType = tjobj[i].get<std::string>();
    outFlags[currentFlagId] = frameString(stringType.c_str());
  }
}

NUMERICAL_DATA_TYPE getNumericalDatatype(const std::string &type) {
  auto found = materialKeys::STRING_TO_NUMERICAL_TYPE.find(type);
  if (found != materialKeys::STRING_TO_NUMERICAL_TYPE.end()) {
    return found->second;
  }
  SE_CORE_ERROR("Invalid numerical string type {}", type);
  return NUMERICAL_DATA_TYPE::UNDEFINED;
}

uint32_t getNumericalTypeSize(NUMERICAL_DATA_TYPE type) {
  auto found = NUMERICAL_TYPE_TO_SIZE.find(type);
  if (found != NUMERICAL_TYPE_TO_SIZE.end()) {
    return found->second;
  }
  SE_CORE_ERROR("Invalid numerical type, cannot look up the size: {}",
                static_cast<uint32_t>(type));
  return 0;
}

void extractDataFromType(const nlohmann::json &jobj,
                         MaterialSourceSubBinding &materialSourceSubBinding,
                         const NUMERICAL_DATA_TYPE type, uint32_t size) {
  switch (type) {
    case NUMERICAL_DATA_TYPE::FLOAT: {
      float data = jobj["value"].get<float>();
      memcpy(&materialSourceSubBinding.value[0], &data, size);
      return;
    }
    case NUMERICAL_DATA_TYPE::INT: {
      int data = jobj["value"].get<int>();
      memcpy(&materialSourceSubBinding.value[0], &data, size);
      return;
    }
    case NUMERICAL_DATA_TYPE::BOOLEAN: {
      bool data = jobj["value"].get<bool>();
      memcpy(&materialSourceSubBinding.value[0], &data, size);
      return;
    }
    case NUMERICAL_DATA_TYPE::VEC2: {
      glm::vec2 data = getValueIfInJson(jobj, "value", glm::vec2{});
      memcpy(&materialSourceSubBinding.value[0], &data, size);
      return;
    }
    case NUMERICAL_DATA_TYPE::VEC3: {
      glm::vec3 data = getValueIfInJson(jobj, "value", glm::vec3{});
      memcpy(&materialSourceSubBinding.value[0], &data, size);
      break;
    }
    case NUMERICAL_DATA_TYPE::VEC4: {
      glm::vec4 data = getValueIfInJson(jobj, "value", glm::vec4{});
      memcpy(&materialSourceSubBinding.value[0], &data, size);
      return;
    }
    case NUMERICAL_DATA_TYPE::MAT3: {
      assert(0);
      return;
    }
    case NUMERICAL_DATA_TYPE::MAT4: {
      glm::mat4 data = getValueIfInJson(jobj, "value", glm::mat4{});
      memcpy(&materialSourceSubBinding.value[0], &data, size);
      return;
    }
    case NUMERICAL_DATA_TYPE::INT16: {
      auto data = jobj["value"].get<int16_t>();
      memcpy(&materialSourceSubBinding.value[0], &data, size);
      return;
    }
    case NUMERICAL_DATA_TYPE::FLOAT16: {
      assert(0);
      return;
    }
    case NUMERICAL_DATA_TYPE::UNDEFINED:
    default: {
      assert(0);
      return;
    }
  }
}

void parseConstantBuffer(MaterialSourceBinding *binding,
                         const nlohmann::json &resource) {
  const auto &content = resource["uniformContent"];
  auto count = static_cast<uint32_t>(content.size());
  auto *uniformData = static_cast<MaterialSourceSubBinding *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(MaterialSourceSubBinding) *
                                              count));
  for (uint32_t i = 0; i < count; ++i) {
    const auto &subResource = content[i];
    const auto &name = subResource["name"].get<std::string>();
    const auto &strType = subResource["type"].get<std::string>();
    NUMERICAL_DATA_TYPE type = getNumericalDatatype(strType);
    uint32_t datasize = getNumericalTypeSize(type);
    assert(type != NUMERICAL_DATA_TYPE::UNDEFINED);
    assert(datasize != 0);
    extractDataFromType(subResource, uniformData[i], type, datasize);
    uniformData[i].type = type;
    uniformData[i].sizeInByte = datasize;
    assert(name.size() <= 31);
    memcpy(uniformData[i].name, name.c_str(), name.size());
    uniformData[i].name[name.size()] = '\0';
  }
  binding->subBinding = uniformData;
  binding->subBindingCount = count;
}

MaterialManager::PreliminaryMaterialParse MaterialManager::parseMaterial(
    const char *path) {
  // for materials we do not perform the check whether is loaded or not
  // each object is going to get it s own material copy.
  // if that starts to be an issue we will add extra logic to deal with this.
  assert(fileExists(path));
  const std::string name = getFileName(path);

  nlohmann::json jobj;
  getJsonObj(path, jobj);
  bool isStatic = getValueIfInJson(jobj, materialKeys::IS_STATIC_KEY, false);

  PreliminaryMaterialParse toReturn;
  toReturn.isStatic = isStatic;
  parseQueueTypeFlags(toReturn.shaderQueueTypeFlagsStr, jobj);

  // parse material resources
  assert(jobj.find("resources") != jobj.end());
  const auto &bindingResources = jobj["resources"];
  auto count = static_cast<uint32_t>(bindingResources.size());
  auto *bindings = static_cast<MaterialSourceBinding *>(
      globals::PERSISTENT_ALLOCATOR->allocate(sizeof(MaterialSourceBinding) *
                                              count));
  for (uint32_t i = 0; i < count; ++i) {
    const auto &subRes = bindingResources[i];
    const auto type = subRes["type"].get<std::string>();
    const auto bName = subRes["bindingName"].get<std::string>();
    const auto resPath = subRes["resourcePath"].get<std::string>();
    bindings[i].type = persistentString(type.c_str());
    bindings[i].bindingName = persistentString(bName.c_str());
    bindings[i].resourcePath = persistentString(resPath.c_str());

    if (strcmp(bindings[i].type, "constantBuffer") == 0) {
      parseConstantBuffer(&bindings[i], subRes);
    }
  }
  toReturn.sourceBindings = bindings;
  toReturn.sourceBindingsCount = count;

  return toReturn;
}

}  // namespace SirEngine
