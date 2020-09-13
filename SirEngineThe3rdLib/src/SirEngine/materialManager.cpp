
#include "SirEngine/materialManager.h"

#include <cassert>
#include <string>
#include <unordered_map>

#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/materialMetadata.h"
#include "SirEngine/log.h"
#include "SirEngine/psoManager.h"
#include "SirEngine/rootSignatureManager.h"
#include "SirEngine/runtimeString.h"
#include "SirEngine/textureManager.h"

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
}  // namespace materialKeys

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

uint32_t findBindingIndex(const graphics::MaterialMetadata *meta,
                          const std::string &bindName) {
  uint32_t count = meta->objectResourceCount;
  for (uint32_t i = 0; i < count; ++i) {
    const auto &resource = meta->objectResources[i];
    bool result = strcmp(resource.name, bindName.c_str()) == 0;
    if (result) {
      return i;
    }
  }
  assert(0 && "could not find binding name");
  return 9999;
}

void MaterialManager::buildBindingTableDefinitionFromMetadta(
    const graphics::MaterialMetadata *meta) {
  uint32_t objectsCount = meta->objectResourceCount;
  // zeroing out memory jsut to be safe
  memset(m_descriptions, 0,
         sizeof(graphics::BindingDescription) * objectsCount);
  for (uint32_t obj = 0; obj < objectsCount; ++obj) {
    const graphics::MaterialResource &res = meta->objectResources[obj];
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

  // NEW code avoiding the old crap
  for (uint32_t i = 0; i < QUEUE_COUNT; ++i) {
    if (!materialData.shaderBindPerQueue[i].pso.isHandleValid()) {
      continue;
    }
    ShaderBind bind = materialData.shaderBindPerQueue[i];

    const graphics::MaterialMetadata *meta =
        globals::PSO_MANAGER->getMetadata(bind.pso);

    buildBindingTableDefinitionFromMetadta(meta);

    std::string bindingName = name + "-bindingTable";
    BindingTableHandle bindingTable =
        globals::BINDING_TABLE_MANAGER->allocateBindingTable(
            m_descriptions, meta->objectResourceCount,
            parse.isStatic
                ? graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_NONE
                : graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
            bindingName.c_str());
    materialData.bindingHandle[i] = bindingTable;

    // update material

    for (uint32_t res = 0; res < parse.sourceBindingsCount; ++res) {
      const MaterialSourceBinding &matBinding = parse.sourceBindings[res];
      const std::string name = getFileName(matBinding.resourcePath);
      if (strcmp(matBinding.type, "texture") == 0) {
        TextureHandle tHandle =
            globals::TEXTURE_MANAGER->loadTexture(matBinding.resourcePath);

        uint32_t bindingIdx = findBindingIndex(meta, matBinding.bindingName);

        globals::BINDING_TABLE_MANAGER->bindTexture(
            materialData.bindingHandle[i], tHandle, bindingIdx, bindingIdx,
            false);
      } else if (strcmp(matBinding.type, "mesh") == 0) {
        MeshHandle mHandle =
            globals::MESH_MANAGER->getHandleFromName(name.c_str());

        globals::BINDING_TABLE_MANAGER->bindMesh(
            materialData.bindingHandle[i], mHandle, meta->meshBinding.binding,
            meta->meshBinding.flags);
      }
    }
  }
  materialData.materialBindingCount = parse.sourceBindingsCount;
  materialData.materialBinding = parse.sourceBindings;

  return handle;
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

MaterialManager::PreliminaryMaterialParse MaterialManager::parseMaterial(
    const char *path) {
  // for materials we do not perform the check whether is loaded or not
  // each object is going to get it s own material copy.
  // if that starts to be an issue we will add extra logic to deal with this.
  assert(fileExists(path));
  const std::string name = getFileName(path);

  auto jobj = getJsonObj(path);
  bool isStatic = getValueIfInJson(jobj, materialKeys::IS_STATIC_KEY, false);

  PreliminaryMaterialParse toReturn;
  toReturn.isStatic = isStatic;
  parseQueueTypeFlags(toReturn.shaderQueueTypeFlagsStr, jobj);

  // parse material resources
  assert(jobj.find("resources") != jobj.end());
  const auto &bindingResources = jobj["resources"];
  uint32_t count = static_cast<uint32_t>(bindingResources.size());
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
  }
  toReturn.sourceBindings = bindings;
  toReturn.sourceBindingsCount = count;

  return toReturn;
}

}  // namespace SirEngine
