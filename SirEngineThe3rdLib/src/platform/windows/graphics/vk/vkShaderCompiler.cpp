#include "platform/windows/graphics/vk/vkShaderCompiler.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/runtimeString.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/volk.h"
#include <cassert>

#undef max
#include "SirEngine/log.h"
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <glslang/Public/ShaderLang.h>

namespace SirEngine::vk {

static const std::unordered_map<SHADER_TYPE, EShLanguage> TYPE_TO_LANGUAGE{
    {SHADER_TYPE::VERTEX, EShLangVertex},
    {SHADER_TYPE::FRAGMENT, EShLangFragment},
    {SHADER_TYPE::COMPUTE, EShLangCompute}};

static const std::unordered_map<SHADER_TYPE, const char *> TYPE_TO_ENTRY_POINT{
    {SHADER_TYPE::VERTEX, "VS"},
    {SHADER_TYPE::FRAGMENT, "PS"},
    {SHADER_TYPE::COMPUTE, "CS"}};

static const std::unordered_map<std::string, SHADER_TYPE> NAME_TO_SHADER_TYPE{
    {"vertex", SHADER_TYPE::VERTEX},
    {"fragment", SHADER_TYPE::FRAGMENT},
    {"compute", SHADER_TYPE::COMPUTE}};

bool getShaderStage(SHADER_TYPE type, EShLanguage &language) {
  const auto found = TYPE_TO_LANGUAGE.find(type);
  if (found != TYPE_TO_LANGUAGE.end()) {
    language = found->second;
    return true;
  }
  return false;
}
static const TBuiltInResource DEFAULT_T_BUILT_IN_RESOURCE = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,

    /* .limits = */
    {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};

VkShaderCompiler::VkShaderCompiler() { glslang::InitializeProcess(); }

VkShaderCompiler::~VkShaderCompiler() { glslang::FinalizeProcess(); }

const char *getEntryPointFromType(SHADER_TYPE type) {
  const auto found = TYPE_TO_ENTRY_POINT.find(type);
  if (found != TYPE_TO_ENTRY_POINT.end()) {
	  return found->second;
  }
  return nullptr;
};

SpirVBlob VkShaderCompiler::compileToSpirV(const char *shaderPath,
                                           VkShaderArgs &shaderArgs,
                                           std::string *log) const {

  uint32_t fileSize;
  const char *fileContent = frameFileLoad(shaderPath, fileSize);

  EShLanguage shaderType;
  getShaderStage(shaderArgs.type, shaderType);
  glslang::TShader shader(shaderType);

  shader.setStrings(&fileContent, 1);
  //TODO investigate why any entry point other than main does not work
  const char *entryPoint = getEntryPointFromType(shaderArgs.type);
  assert(entryPoint != nullptr);
  shader.setEntryPoint(entryPoint);
  shader.setSourceEntryPoint(entryPoint);

  int clientInputSemanticsVersion = 110; // maps to, say, #define VULKAN 110
  glslang::EShTargetClientVersion vulkanClientVersion =
      glslang::EShTargetVulkan_1_1;
  //glslang::EShTargetLanguageVersion targetVersion = glslang::EShTargetSpv_1_3;

  shader.setEnvInput(glslang::EShSourceGlsl, shaderType,
                     glslang::EShClientVulkan, clientInputSemanticsVersion);
  shader.setEnvClient(glslang::EShClientVulkan, vulkanClientVersion);
  //shader.setEnvTarget(glslang::EShTargetSpv, targetVersion);

  TBuiltInResource resources = DEFAULT_T_BUILT_IN_RESOURCE;
  auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

  const int defaultVersion = 100;

  DirStackFileIncluder includer;

  // Get Path of File
  std::string path = getPathName(shaderPath);
  includer.pushExternalLocalDirectory(path);

  std::string preprocessedGlsl;

  if (!shader.preprocess(&resources, defaultVersion, ENoProfile, false, false,
                         messages, &preprocessedGlsl, includer)) {
    SE_CORE_ERROR(
        "GLSL Preprocessing Failed for: {0}  \n LOG: {1} \n DEBUG LOG: {2}",
        shaderPath, shader.getInfoLog(), shader.getInfoDebugLog());
    if (log != nullptr) {
      (*log) += "GLSL Preprocessing Failed for: ";
      (*log) += shaderPath;
      (*log) += " \n LOG: ";
      (*log) += shader.getInfoLog();
      (*log) += "Debug LOG: ";
      (*log) += shader.getInfoDebugLog();
    }
    return {nullptr, 0};
  }
  const char *preprocessedCStr = preprocessedGlsl.c_str();
  shader.setStrings(&preprocessedCStr, 1);

  if (!shader.parse(&resources, 100, false, messages)) {
    SE_CORE_ERROR("GLSL parsing Failed for: {0}  \n LOG: {1} \n DEBUG LOG: {2}",
                  shaderPath, shader.getInfoLog(), shader.getInfoDebugLog());
    if (log != nullptr) {
      (*log) += "GLSL parsing Failed for: ";
      (*log) += shaderPath;
      (*log) += " \n LOG: ";
      (*log) += shader.getInfoLog();
      (*log) += "Debug LOG: ";
      (*log) += shader.getInfoDebugLog();
    }
    return {nullptr, 0};
  }
  glslang::TProgram program;
  program.addShader(&shader);

  if (!program.link(messages)) {
    SE_CORE_ERROR("GLSL linking Failed for: {0}  \n LOG: {1} \n DEBUG LOG: {2}",
                  shaderPath, program.getInfoLog(), program.getInfoDebugLog());
    if (log != nullptr) {
      (*log) += "GLSL parsing Failed for: ";
      (*log) += shaderPath;
      (*log) += " \n LOG: ";
      (*log) += program.getInfoLog();
      (*log) += "Debug LOG: ";
      (*log) += program.getInfoDebugLog();
    }
    return {nullptr, 0};
  }
  // Not really happy about having an std::vector at interface
  std::vector<unsigned int> spirV;
  spv::SpvBuildLogger logger;
  glslang::SpvOptions spvOptions;
  glslang::GlslangToSpv(*program.getIntermediate(shaderType), spirV, &logger,
                        &spvOptions);

  auto sizeInByte = static_cast<uint32_t>(spirV.size() * sizeof(uint32_t));
  void *blobMemory = globals::FRAME_ALLOCATOR->allocate(sizeInByte);
  memcpy(blobMemory, spirV.data(), sizeInByte);
  return {blobMemory, static_cast<uint32_t>(sizeInByte)};
}

VkShaderModule VkShaderCompiler::spirvToShaderModule(const SpirVBlob &blob) {
  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  createInfo.codeSize = blob.sizeInByte;
  createInfo.pCode = reinterpret_cast<uint32_t *>(blob.memory);

  VkShaderModule shaderModule = nullptr;
  VK_CHECK(vkCreateShaderModule(vk::LOGICAL_DEVICE, &createInfo, nullptr,
                                &shaderModule));
  return shaderModule;
}

VkShaderModule VkShaderCompiler::compileToShaderModule(const char *shaderPath,
                                                       VkShaderArgs &shaderArgs,
                                                       std::string *log) const {
  const SpirVBlob shader = compileToSpirV(shaderPath, shaderArgs, log);
  if(shader.memory == nullptr || shader.sizeInByte ==0)
  {
      return nullptr;
  }
  return spirvToShaderModule(shader);
}
uint32_t VkShaderCompiler::getShaderFlags(const VkShaderArgs &shaderArgs) {
  uint32_t flags = 0;
  flags |= (shaderArgs.debug ? SHADER_FLAGS::DEBUG : 0);

  return flags;
}

SHADER_TYPE VkShaderCompiler::getShaderTypeFromName(const char *name) {
  const auto found = NAME_TO_SHADER_TYPE.find(name);
  if (found != NAME_TO_SHADER_TYPE.end()) {
    return found->second;
  }
  return SHADER_TYPE::INVALID;
}
} // namespace SirEngine::vk
