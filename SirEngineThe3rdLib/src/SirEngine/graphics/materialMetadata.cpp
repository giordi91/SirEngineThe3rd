#include "SirEngine/graphics/materialMetadata.h"

#include <assert.h>

#include <SPIRV-CROSS/spirv_cross.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/cpu/stackAllocator.h"
#include "SirEngine/psoManager.h"
#include "platform/windows/graphics/dx12/shaderCompiler.h"
#include "nlohmann/json.hpp"

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

// TODO ideally this path would come from the engine config, unluckily this code
// is mostly called by the offline compiler where there is not an engine config
// providing the paths this is not ideal and we might want to sort it out in the
// future
static const std::string RASTERIZATION_PATH =
    "../data/shaders/VK/rasterization/";
static const std::string RASTERIZATION_PATH_HLSL =
    "../data/shaders/DX12/rasterization/";
static const std::string COMPUTE_PATH = "../data/shaders/VK/compute/";
static const std::string DX12_COMPUTE_PATH = "../data/shaders/DX12/compute/";
static const std::string GLSL_EXTENSION = ".glsl";
static const std::string HLSL_EXTENSION = ".hlsl";
static const std::string VULKAN_PROCESSED_PSO_PATH =
    "../data/processed/pso/VK/";
static const std::string DX12_PROCESSED_PSO_PATH =
    "../data/processed/pso/DX12/";
static const std::string METADATA_EXTENSION = ".metadata";
;

static const std::unordered_map<std::string, MATERIAL_RESOURCE_FLAGS>
    NAME_TO_MESH_FLAG{
        {PSO_MESH_VERTICES_KEY, MATERIAL_RESOURCE_FLAGS::MESH_VERTICES},
        {PSO_MESH_NORMALS_KEY, MATERIAL_RESOURCE_FLAGS::MESH_NORMALS},
        {PSO_MESH_UVS_KEY, MATERIAL_RESOURCE_FLAGS::MESH_UVS},
        {PSO_MESH_TANGENTS_KEY, MATERIAL_RESOURCE_FLAGS::MESH_TANGENTS},
    };

MATERIAL_RESOURCE_FLAGS getMeshFlags(const std::string &name) {
  const auto found = NAME_TO_MESH_FLAG.find(name);
  if (found != NAME_TO_MESH_FLAG.end()) {
    return found->second;
  }
  return MATERIAL_RESOURCE_FLAGS::NONE;
}

NUMERICAL_DATA_TYPE getDatatype(const spirv_cross::SPIRType::BaseType base,
                                const int width, const uint32_t set) {
  switch (base) {
    case spirv_cross::SPIRType::Boolean: {
      return NUMERICAL_DATA_TYPE::BOOLEAN;
    }
    case spirv_cross::SPIRType::Short:
    case spirv_cross::SPIRType::UShort: {
      return NUMERICAL_DATA_TYPE::INT16;
    }
    case spirv_cross::SPIRType::Int:
    case spirv_cross::SPIRType::UInt: {
      return NUMERICAL_DATA_TYPE::INT;
    }
    case spirv_cross::SPIRType::Half: {
      return NUMERICAL_DATA_TYPE::FLOAT16;
    }
    case spirv_cross::SPIRType::Float: {
      switch (width) {
        case (1): {
          return NUMERICAL_DATA_TYPE::FLOAT;
        }
        case (2): {
          return NUMERICAL_DATA_TYPE::VEC2;
        }
        case (3): {
          return NUMERICAL_DATA_TYPE::VEC3;
        }
        case (4): {
          return NUMERICAL_DATA_TYPE::VEC4;
        }
        case (9): {
          return NUMERICAL_DATA_TYPE::MAT3;
        }
        case (16): {
          return NUMERICAL_DATA_TYPE::MAT4;
        }
        default: {
          if (set == 3) {
            SE_CORE_ERROR(
                "Unsupported width for float type in constant buffer type {}",
                width);
          }
          return NUMERICAL_DATA_TYPE::UNDEFINED;
        }
      }
    }
    case spirv_cross::SPIRType::Struct: {
      if (set == 3) {
        SE_CORE_ERROR(
            "Unsupported nested struct  in constant buffer in object space "
            "reflection");
      }
      return NUMERICAL_DATA_TYPE::UNDEFINED;
    }

    case spirv_cross::SPIRType::Int64:
    case spirv_cross::SPIRType::UInt64:
    case spirv_cross::SPIRType::AtomicCounter:
    case spirv_cross::SPIRType::Double:

    case spirv_cross::SPIRType::Image:
    case spirv_cross::SPIRType::SampledImage:
    case spirv_cross::SPIRType::Sampler:
    case spirv_cross::SPIRType::AccelerationStructure:
    case spirv_cross::SPIRType::RayQuery:
    case spirv_cross::SPIRType::ControlPointArray:
    case spirv_cross::SPIRType::Char:
    case spirv_cross::SPIRType::Unknown:
    case spirv_cross::SPIRType::Void:
    case spirv_cross::SPIRType::SByte:
    case spirv_cross::SPIRType::UByte:
    default: {
      SE_CORE_ERROR("Unsupported base type in constant buffer type {}",
                    static_cast<uint32_t>(spirv_cross::SPIRType::UByte));
      assert(0);
      return NUMERICAL_DATA_TYPE::UNDEFINED;
    }
  }
}

MaterialMetadataUniform extractUniformBufferOffset(
    spirv_cross::Compiler &comp, const spirv_cross::Resource &uniform,
    const uint32_t set) {
  const spirv_cross::SPIRType &type = comp.get_type(uniform.type_id);

  // here should be the outer block, we want to go to the inner type;
  // spirv_cross::SPIRType strType = comp.get_type(type.member_types[0]);
  spirv_cross::SPIRType strType = type;
  // const std::string &strName = comp.get_name(type.member_types[0]);
  const std::string &strName = comp.get_name(uniform.id);

  auto count = static_cast<uint32_t>(strType.member_types.size());

  auto *outStructMember = static_cast<MaterialMetadataStructMember *>(
      globals::PERSISTENT_ALLOCATOR->allocate(
          count * sizeof(MaterialMetadataStructMember)));

  size_t structSize = comp.get_declared_struct_size(strType);
  // here we iterate all the members and extract the necessary information for
  // each of them
  for (uint32_t i = 0; i < count; ++i) {
    const spirv_cross::SPIRType &memberType =
        comp.get_type(strType.member_types[i]);
    size_t memberSize = comp.get_declared_struct_member_size(strType, i);

    // Get member offset within this struct.
    size_t offset = comp.type_struct_member_offset(strType, i);
    const std::string &name = comp.get_member_name(strType.self, i);

    // copying the name
    assert(name.size() <= 31);
    outStructMember[i].offset = static_cast<uint32_t>(offset);

    // populate the rest of the tracker, with all the necessary information
    outStructMember[i].size = static_cast<uint32_t>(memberSize);
    outStructMember[i].datatype = getDatatype(
        memberType.basetype, memberType.vecsize * memberType.columns, set);
    memcpy(&outStructMember[i].name[0], name.c_str(), name.size());
    outStructMember[i].name[name.size()] = '\0';
  }

  MaterialMetadataUniform toReturn{};
  toReturn.members = outStructMember;
  toReturn.membersCount = static_cast<uint32_t>(count);

  std::string name = strName;
  memcpy(&toReturn.name[0], name.c_str(), name.size());
  toReturn.name[name.size()] = '\0';
  toReturn.structSize = static_cast<uint32_t>(structSize);
  return toReturn;
}

MaterialMetadata extractMetadataFromShader(const char *shaderName,
                                           SHADER_TYPE type) {
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

  SirEngine::dx12::DXCShaderCompiler compiler;

  std::string log;
  dx12::ShaderArgs args;
  args.debug = true;
  if (type == SHADER_TYPE::VERTEX) {
    args.type = L"vs_6_2";
    args.entryPoint = L"VS";
  }
  if (type == SHADER_TYPE::FRAGMENT) {
    args.type = L"ps_6_2";
    args.entryPoint = L"PS";
  }
  if (type == SHADER_TYPE::COMPUTE) {
    args.type = L"cs_6_2";
    args.entryPoint = L"CS";
  }
  dx12::ShaderCompileResult result = compiler.toSpirv(shaderName, args);
  auto blob = result.blob;
  auto sizeInByte = blob->GetBufferSize();

  std::vector<unsigned int> spirV;
  spirV.resize(sizeInByte / 4);
  memcpy(spirV.data(), blob->GetBufferPointer(), sizeInByte);
  spirv_cross::Compiler comp(spirV);

  spirv_cross::ShaderResources res = comp.get_shader_resources();

  auto totalCount = static_cast<uint32_t>(
      res.separate_images.size() + res.storage_buffers.size() +
      res.uniform_buffers.size() + res.push_constant_buffers.size());

  uint32_t allocSize = sizeof(MaterialResource) * totalCount;
  auto *memory = static_cast<MaterialResource *>(
      globals::FRAME_ALLOCATOR->allocate(allocSize));

  int counter = 0;

  for (const auto &image : res.separate_images) {
    auto set = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationDescriptorSet));
    auto binding = static_cast<uint16_t>(
        comp.get_decoration(image.id, spv::DecorationBinding));

    memory[counter] = {{},
                       {},
                       MATERIAL_RESOURCE_TYPE::TEXTURE,
                       visibility,
                       MATERIAL_RESOURCE_FLAGS::NONE,
                       set,
                       binding};
    assert(image.name.size() <= 31);
    memcpy(memory[counter].name, image.name.c_str(), image.name.size());
    memory[counter].name[image.name.size()] = '\0';
    counter += 1;
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

    memory[counter] = {{},         {},    MATERIAL_RESOURCE_TYPE::BUFFER,
                       visibility, flags, set,
                       binding};
    assert(buff.name.size() <= 31);
    memcpy(memory[counter].name, buff.name.c_str(), buff.name.size());
    memory[counter].name[buff.name.size()] = '\0';
    counter += 1;
  }

  // we either have one push constant only or none
  assert(res.push_constant_buffers.size() == 1 ||
         res.push_constant_buffers.size() == 0);
  for (const auto &push : res.push_constant_buffers) {
    // push constants don't really have a binding set, and they translate
    // to constant buffers in hlsl, so we set them to space object and binding
    // 0.
    // this means if a push constant appears must appear at index 0
    uint16_t set = PSOManager::PER_OBJECT_BINDING_INDEX;
    uint16_t binding = 0;

    // we need to extract the offset of the datatype
    MaterialMetadataUniform uniformMetadata =
        extractUniformBufferOffset(comp, push, set);
    auto a = comp.get_name(push.base_type_id);
    std::string name = comp.get_name(push.id);

    memory[counter] = {{},
                       uniformMetadata,
                       MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER,
                       visibility,
                       MATERIAL_RESOURCE_FLAGS::PUSH_CONSTANT_BUFFER,
                       set,
                       binding};

    assert(name.size() <= 31);
    memcpy(memory[counter].name, name.c_str(), name.size());
    memory[counter].name[name.size()] = '\0';
    counter += 1;
  }

  for (const auto &uniform : res.uniform_buffers) {
    auto set = static_cast<uint16_t>(
        comp.get_decoration(uniform.id, spv::DecorationDescriptorSet));
    auto binding = static_cast<uint16_t>(
        comp.get_decoration(uniform.id, spv::DecorationBinding));

    // we need to extract the offset of the datatype
    MaterialMetadataUniform uniformMetadata =
        extractUniformBufferOffset(comp, uniform, set);
    auto a = comp.get_name(uniform.base_type_id);
    std::string name = comp.get_name(uniform.id);

    memory[counter] = {{},
                       uniformMetadata,
                       MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER,
                       visibility,
                       MATERIAL_RESOURCE_FLAGS::NONE,
                       set,
                       binding};

    // std::string name = std::regex_replace(
    //    uniform.name, std::regex("type.ConstantBuffer."), "");
    assert(name.size() <= 31);
    memcpy(memory[counter].name, name.c_str(), name.size());
    memory[counter].name[name.size()] = '\0';
    counter += 1;
  }

  return {memory, nullptr, nullptr, static_cast<uint32_t>(totalCount), 0, 0};
}

MaterialMetadata processComputeMetadata(const nlohmann::json &jobj) {
  assertInJson(jobj, PSO_CS_KEY);

  const std::string &name = getValueIfInJson(jobj, PSO_CS_KEY, DEFAULT_STRING);
  const std::string path = DX12_COMPUTE_PATH + name + HLSL_EXTENSION;
  assert(fileExists(path));
  MaterialMetadata meta =
      extractMetadataFromShader(path.c_str(), SHADER_TYPE::COMPUTE);

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
                                     const int count) {
  // we want to build an array of resources bindings, one per mesh binding type
  // we next want to check they have consecutive values, because it is what the
  // binding system expects

  int slots[4] = {-1, -1, -1, -1};
  int slotsDx[4] = {-1, -1, -1, -1};
  uint32_t outFlags = 0;

  int counter = 0;
  for (int i = 0; i < count; ++i) {
    const auto &meta = vsMeta[i];
    auto flags = static_cast<uint32_t>(meta.flags);
    if ((flags &
         static_cast<uint32_t>(MATERIAL_RESOURCE_FLAGS::MESH_VERTICES)) > 0) {
      assert(counter < 4);
      slots[counter] = meta.binding;
      slotsDx[counter] = i;
      ++counter;
      outFlags |= MESH_ATTRIBUTE_FLAGS::POSITIONS;
    }
    if ((flags & static_cast<uint32_t>(MATERIAL_RESOURCE_FLAGS::MESH_NORMALS)) >
        0) {
      assert(counter < 4);
      slots[counter] = meta.binding;
      slotsDx[counter] = i;
      ++counter;
      outFlags |= MESH_ATTRIBUTE_FLAGS::NORMALS;
    }
    if ((flags & static_cast<uint32_t>(MATERIAL_RESOURCE_FLAGS::MESH_UVS)) >
        0) {
      assert(counter < 4);
      slots[counter] = meta.binding;
      slotsDx[counter] = i;
      ++counter;
      outFlags |= MESH_ATTRIBUTE_FLAGS::UV;
    }
    if ((flags &
         static_cast<uint32_t>(MATERIAL_RESOURCE_FLAGS::MESH_TANGENTS)) > 0) {
      assert(counter < 4);
      slots[counter] = meta.binding;
      slotsDx[counter] = i;
      ++counter;
      outFlags |= MESH_ATTRIBUTE_FLAGS::TANGENTS;
    }
  }
  // return an empty slot in case we have no geometry bound
  if (slots[0] == -1) {
    return {-1, -1, MESH_ATTRIBUTE_NONE};
  }

  // lets validate the assumption that they are consecutive
  for (int i = 1; i < counter; ++i) {
    int prev = slots[i - 1];
    int curr = slots[i];
    if ((curr - prev) != 1) {
      SE_CORE_ERROR("Mesh validation failed for vk bindings {}", name);
      SE_CORE_ERROR(
          "expected consecutive slots but found previous {} and current {}",
          prev, curr);
    }
  }
  // lets validate the assumption that they are consecutive
  for (int i = 1; i < counter; ++i) {
    int prev = slotsDx[i - 1];
    int curr = slotsDx[i];
    if ((curr - prev) != 1) {
      SE_CORE_ERROR("Mesh validation failed for Dx bindings {}", name);
      SE_CORE_ERROR(
          "expected consecutive slots but found previous {} and current {}",
          prev, curr);
    }
  }
  return {slotsDx[0], slots[0], static_cast<MESH_ATTRIBUTE_FLAGS>(outFlags)};
}

void validatePushConstants(MaterialResource *space3, int counter) {
  // find lets find push constants

  auto expectedFlag =
      static_cast<uint32_t>(MATERIAL_RESOURCE_FLAGS::PUSH_CONSTANT_BUFFER);
  int pushIndex = -1;
  for (int i = 0; i < counter; ++i) {
    const auto &meta = space3[i];
    auto sourceFlag = static_cast<uint32_t>(meta.flags);
    if ((sourceFlag & expectedFlag) != 0) {
      pushIndex = i;
      break;
    }
  }
  if (pushIndex == -1) {
    return;
  }

  // now let us iterate all the resources and make sure that they don't have
  // index 0
  for (int i = 0; i < counter; ++i) {
    const auto &meta = space3[i];
    // skipping the push constant itself
    if (i == pushIndex) {
      continue;
    }
    if (meta.binding == 0) {
      SE_CORE_ERROR(
          "cannot have push constant and a resource bound to index 0");
      assert(0 && "cannot have push constant and a resource bound to index 0");
    }
  }

};  // namespace SirEngine::graphics

MaterialMetadata processRasterMetadata2(const char *path,
                                        const nlohmann::json &jobj) {
  assertInJson(jobj, PSO_VS_KEY);

  const std::string &vsName =
      getValueIfInJson(jobj, PSO_VS_KEY, DEFAULT_STRING);
  const std::string &psName =
      getValueIfInJson(jobj, PSO_PS_KEY, DEFAULT_STRING);

  // Not ideal to concatenate strings like that but, this is for offline
  // processing so I am not worrying too much about it
  const std::string vsPath = RASTERIZATION_PATH_HLSL + vsName + ".hlsl";
  assert(fileExists(vsPath));
  MaterialMetadata vsMeta =
      extractMetadataFromShader(vsPath.c_str(), SHADER_TYPE::VERTEX);

  MaterialMetadata psMeta = {};
  // Fragment shader might be empty, this is the case because fragment might
  // be optional in some shaders like shadow mapping, where not having
  // fragment shader allows the card to go to the fast path
  if (!psName.empty()) {
    const std::string psPath = RASTERIZATION_PATH_HLSL + psName + ".hlsl";
    assert(fileExists(psPath));
    psMeta = extractMetadataFromShader(psPath.c_str(), SHADER_TYPE::FRAGMENT);
  }

  // we want to merge both vertex and fragment shader data and remove
  // duplicates
  MaterialResource space0[16];
  MaterialResource space2[16];
  MaterialResource space3[32];
  MaterialResource *resources[4] = {&space0[0], nullptr, &space2[0],
                                    &space3[0]};
  int counters[4] = {0, 0, 0, 0};
  int maxCounters[4] = {16, 0, 16, 32};
  for (uint32_t i = 0; i < vsMeta.objectResourceCount; ++i) {
    MaterialResource &res = vsMeta.objectResources[i];
    resources[res.set][counters[res.set]++] = res;
    assert(counters[res.set] < maxCounters[res.set]);
  }
  // merging in ps
  if (!psName.empty()) {
    for (uint32_t i = 0; i < psMeta.objectResourceCount; ++i) {
      MaterialResource &res = psMeta.objectResources[i];
      // check if is unique
      int currCounter = counters[res.set];
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
        // we don't extract metadata for materials constant buffer which are
        // not on a per object space
        if (res.type == MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER) {
          // free the unused uniform metadata
          globals::PERSISTENT_ALLOCATOR->free(res.extension.uniform.members);
        }
        continue;
      }
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

  // validate mesh bindings
  MaterialMeshBinding meshBinding = validateMeshData(path, space3, counters[3]);

  // let us validate push constants
  validatePushConstants(space3, counters[3]);

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

MaterialMetadata processRasterMetadata(const char *path,
                                       const nlohmann::json &jobj) {
  assertInJson(jobj, PSO_VS_KEY);

  const std::string &vsName =
      getValueIfInJson(jobj, PSO_VS_KEY, DEFAULT_STRING);
  const std::string &psName =
      getValueIfInJson(jobj, PSO_PS_KEY, DEFAULT_STRING);

  // Not ideal to concatenate strings like that but, this is for offline
  // processing so I am not worrying too much about it
  const std::string vsPath = RASTERIZATION_PATH + vsName + GLSL_EXTENSION;
  assert(fileExists(vsPath));
  MaterialMetadata vsMeta =
      extractMetadataFromShader(vsPath.c_str(), SHADER_TYPE::VERTEX);

  MaterialMetadata psMeta = {};
  // Fragment shader might be empty, this is the case because fragment might
  // be optional in some shaders like shadow mapping, where not having
  // fragment shader allows the card to go to the fast path
  if (!psName.empty()) {
    const std::string psPath = RASTERIZATION_PATH + psName + GLSL_EXTENSION;
    assert(fileExists(psPath));
    psMeta = extractMetadataFromShader(psPath.c_str(), SHADER_TYPE::FRAGMENT);
  }

  // we want to merge both vertex and fragment shader data and remove
  // duplicates
  MaterialResource space0[16];
  MaterialResource space2[16];
  MaterialResource space3[32];
  MaterialResource *resources[4] = {&space0[0], nullptr, &space2[0],
                                    &space3[0]};
  int counters[4] = {0, 0, 0, 0};
  int maxCounters[4] = {16, 0, 16, 32};
  for (uint32_t i = 0; i < vsMeta.objectResourceCount; ++i) {
    MaterialResource &res = vsMeta.objectResources[i];
    resources[res.set][counters[res.set]++] = res;
    assert(counters[res.set] < maxCounters[res.set]);
  }
  // merging in ps
  if (!psName.empty()) {
    for (uint32_t i = 0; i < psMeta.objectResourceCount; ++i) {
      MaterialResource &res = psMeta.objectResources[i];
      // check if is unique
      int currCounter = counters[res.set];
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
        // we don't extract metadata for materials constant buffer which are
        // not on a per object space
        if (res.type == MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER) {
          // free the unused uniform metadata
          globals::PERSISTENT_ALLOCATOR->free(res.extension.uniform.members);
        }
        continue;
      }
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

MaterialMetadata extractMetadataFromPSO(const char *psoPath) {
  nlohmann::json jobj;
  getJsonObj(psoPath, jobj);
  assertInJson(jobj, PSO_TYPE_KEY);

  const std::string &psoType = jobj[PSO_TYPE_KEY].get<std::string>();
  if (psoType == PSO_TYPE_RASTER) {
    return processRasterMetadata2(psoPath, jobj);
  }
  return processComputeMetadata(jobj);
}

MaterialMetadata loadPSOBinaryMetadata(const char *psoPath) {
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

  // coping the data to the persistent memory
  memcpy(outData.objectResources, data + mapper->objectResourceDataOffset,
         objectSize);
  memcpy(outData.frameResources, data + mapper->frameResourceDataOffset,
         frameSize);
  memcpy(outData.passResources, data + mapper->passResourceDataOffset,
         passSize);

  // first of all we load in memory the whole block of uniforms metadata
  auto uniformSize = mapper->objectResourceDataOffset;
  if (uniformSize != 0) {
    auto *uniformData = static_cast<char *>(
        globals::PERSISTENT_ALLOCATOR->allocate(uniformSize));
    memcpy(uniformData, data, uniformSize);
    for (uint32_t i = 0; i < mapper->objectResourceCount; ++i) {
      MaterialResource &res = outData.objectResources[i];
      if (res.type == MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER) {
        // we need to patch the pointer
        auto offset = reinterpret_cast<size_t>(res.extension.uniform.members);
        res.extension.uniform.members =
            reinterpret_cast<MaterialMetadataStructMember *>(uniformData +
                                                             offset);
      }
    }
  }
  // patching the count of objects
  outData.objectResourceCount = mapper->objectResourceCount;
  outData.frameResourceCount = mapper->frameResourceCount;
  outData.passResourceCount = mapper->passResourceCount;
  outData.meshBinding =
      MaterialMeshBinding{mapper->dxMeshBinding, mapper->vkMeshBinding,
                          static_cast<MESH_ATTRIBUTE_FLAGS>(mapper->meshFlags)};

  return outData;
}

graphics::MaterialMetadata loadMetadata(const char *psoPath,
                                        const GRAPHIC_API api) {
  std::filesystem::path p(psoPath);
  p.replace_extension(METADATA_EXTENSION);
  std::string finalP;
  if (api == GRAPHIC_API::VULKAN) {
    finalP = VULKAN_PROCESSED_PSO_PATH + p.filename().string();
  } else {
    finalP = DX12_PROCESSED_PSO_PATH + p.filename().string();
  }
  // if the file exists it means we have the necessary binary metadata offline
  // compiled if not, we extract it again, although is slow, we should not do
  // it at runtime, but we allow it for faster iteration
  if (std::filesystem::exists(finalP)) {
    return graphics::loadPSOBinaryMetadata(finalP.c_str());
  }
  SE_CORE_WARN(
      "Could not find compiled metadata for given PSO: {}\nextracting from "
      "source",
      psoPath);
  return graphics::extractMetadataFromPSO(psoPath);
}

}  // namespace SirEngine::graphics
