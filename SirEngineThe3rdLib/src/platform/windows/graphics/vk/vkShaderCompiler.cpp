#include "platform/windows/graphics/vk/vkShaderCompiler.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/runtimeString.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/volk.h"
#include <cassert>

#undef max
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <glslang/Public/ShaderLang.h>

namespace SirEngine::vk {
EShLanguage GetShaderStage(const std::string &stage) {
  if (stage == ".vert") {
    return EShLangVertex;
  } else if (stage == ".tesc") {
    return EShLangTessControl;
  } else if (stage == ".tese") {
    return EShLangTessEvaluation;
  } else if (stage == ".geom") {
    return EShLangGeometry;
  } else if (stage == ".frag") {
    return EShLangFragment;
  } else if (stage == ".comp") {
    return EShLangCompute;
  } else {
    assert(0 && "Unknown shader stage");
    return EShLangCount;
  }
}
const TBuiltInResource DefaultTBuiltInResource = {
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

VkShaderModule VkShaderCompiler::compileShader(const char *shaderPath,
                                               VkShaderArgs &shaderArgs,
                                               std::string *log) {

  uint32_t fileSize;
  const char *program = frameFileLoad(shaderPath, fileSize);

  const std::string extension = getFileExtension(shaderPath);
  EShLanguage shaderType = GetShaderStage(extension);
  glslang::TShader Shader(shaderType);

  Shader.setStrings(&program, 1);

  int ClientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100
  glslang::EShTargetClientVersion VulkanClientVersion =
      glslang::EShTargetVulkan_1_0;
  glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;

  Shader.setEnvInput(glslang::EShSourceGlsl, shaderType,
                     glslang::EShClientVulkan, ClientInputSemanticsVersion);
  Shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
  Shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);

  TBuiltInResource Resources;
  Resources = DefaultTBuiltInResource;
  // Resources = DefaultTBuiltInResource;
  EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

  const int DefaultVersion = 100;

  DirStackFileIncluder Includer;

  // Get Path of File
  std::string Path = getPathName(shaderPath);
  Includer.pushExternalLocalDirectory(Path);

  std::string PreprocessedGLSL;

  if (!Shader.preprocess(&Resources, DefaultVersion, ENoProfile, false, false,
                         messages, &PreprocessedGLSL, Includer)) {
    std::cout << "GLSL Preprocessing Failed for: " << shaderPath << std::endl;
    std::cout << Shader.getInfoLog() << std::endl;
    std::cout << Shader.getInfoDebugLog() << std::endl;
  }
  const char *PreprocessedCStr = PreprocessedGLSL.c_str();
  Shader.setStrings(&PreprocessedCStr, 1);

  if (!Shader.parse(&Resources, 100, false, messages)) {
    std::cout << "GLSL Parsing Failed for: " << shaderPath << std::endl;
    std::cout << Shader.getInfoLog() << std::endl;
    std::cout << Shader.getInfoDebugLog() << std::endl;
  }
  glslang::TProgram Program;
  Program.addShader(&Shader);

  if (!Program.link(messages)) {
    std::cout << "GLSL Linking Failed for: " << shaderPath << std::endl;
    std::cout << Shader.getInfoLog() << std::endl;
    std::cout << Shader.getInfoDebugLog() << std::endl;
  }

  std::vector<unsigned int> SpirV;
  spv::SpvBuildLogger logger;
  glslang::SpvOptions spvOptions;
  glslang::GlslangToSpv(*Program.getIntermediate(shaderType), SpirV, &logger,
                        &spvOptions);

  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  createInfo.codeSize = SpirV.size() * sizeof(int);
  createInfo.pCode = reinterpret_cast<uint32_t *>(SpirV.data());

  VkShaderModule shaderModule = nullptr;
  vkCreateShaderModule(vk::LOGICAL_DEVICE, &createInfo, nullptr, &shaderModule);

  return shaderModule;
}
} // namespace SirEngine::vk
