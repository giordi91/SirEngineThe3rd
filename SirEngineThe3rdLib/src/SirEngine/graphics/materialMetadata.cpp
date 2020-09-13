#include "SirEngine/graphics/materialMetadata.h"

#include <SPIRV-CROSS/spirv_cross.hpp>
#include <cassert>
#include <string>
#include <unordered_map>

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/cpu/stackAllocator.h"
#include "SirEngine/runtimeString.h"
#include "platform/windows/graphics/vk/vkShaderCompiler.h"

namespace SirEngine::graphics {

static const std::string PSO_MESH_VERTICES_KEY = "vertices";
static const std::string PSO_MESH_NORMALS_KEY = "normals";
static const std::string PSO_MESH_UVS_KEY = "uvs";
static const std::string PSO_MESH_TANGENTS_KEY = "tangents";
static const std::string PSO_VS_KEY = "VS";
static const std::string PSO_PS_KEY = "PS";
static const std::string PSO_CS_KEY = "shaderName";
static const std::string DEFAULT_STRING = "";
static const std::string PSO_TYPE_KEY = "type";
static const std::string PSO_TYPE_RASTER = "RASTER";
static const std::string PSO_TYPE_COMPUTE = "COMPUTE";

static const std::unordered_map<std::string, MATERIAL_RESOURCE_FLAGS>
    nameToMeshFlag{
        {PSO_MESH_VERTICES_KEY, MATERIAL_RESOURCE_FLAGS::MESH_VERTICES},
        {PSO_MESH_NORMALS_KEY, MATERIAL_RESOURCE_FLAGS::MESH_NORMALS},
        {PSO_MESH_UVS_KEY, MATERIAL_RESOURCE_FLAGS::MESH_UVS},
        {PSO_MESH_TANGENTS_KEY, MATERIAL_RESOURCE_FLAGS::MESH_TANGENTS},
    };

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
  vk::VkShaderArgs args{true, type};
  vk::SpirVBlob blob = vkCompiler.compileToSpirV(shaderName, args, &log);

  std::vector<unsigned int> spirV;
  spirV.resize(blob.sizeInByte / 4);
  memcpy(spirV.data(), blob.memory, blob.sizeInByte);
  spirv_cross::Compiler comp(spirV);

  spirv_cross::ShaderResources res = comp.get_shader_resources();

  uint32_t totalCount = static_cast<uint32_t>(res.separate_images.size() +
                                                  res.storage_buffers.size() +
                                                  res.uniform_buffers.size());
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

  const std::string &name = getValueIfInJson(jobj, PSO_CS_KEY, DEFAULT_STRING);
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
      getValueIfInJson(jobj, PSO_VS_KEY, DEFAULT_STRING);
  const std::string &psName =
      getValueIfInJson(jobj, PSO_PS_KEY, DEFAULT_STRING);
  const std::string vsPath =
      "../data/shaders/VK/rasterization/" + vsName + ".glsl";
  assert(fileExists(vsPath));
  MaterialMetadata vsMeta = processShader(vsPath.c_str(), SHADER_TYPE::VERTEX);

  MaterialMetadata psMeta = {};
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
      for (int bindIdx = 0; bindIdx < currCounter; ++bindIdx) {
        auto &currRes = resources[res.set][bindIdx];
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

MaterialMetadata loadBinaryMetadata(const char *psoPath) {
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
    auto len = static_cast<uint32_t>(strlen(data + offset));
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

graphics::MaterialMetadata loadMetadata(const char *psoPath,
                                        const GRAPHIC_API api) {
  std::filesystem::path p(psoPath);
  p.replace_extension(".metadata");
  std::string finalP;
  if (api == GRAPHIC_API::VULKAN) {
    finalP = std::string("../data/processed/pso/VK/") + p.filename().string();
  } else {
    finalP = std::string("../data/processed/pso/DX12/") + p.filename().string();
  }
  if (std::filesystem::exists(finalP)) {
    return graphics::loadBinaryMetadata(finalP.c_str());
  }
  SE_CORE_WARN(
      "Could not find compiled metadata for given PSO: {}\nextracting from "
      "source",
      psoPath);
  return graphics::extractMetadata(psoPath);
}

}  // namespace SirEngine::graphics