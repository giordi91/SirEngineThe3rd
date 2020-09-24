#include "vkShaderCompilerPlugin.h"

#include <filesystem>

#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/graphics/graphicsDefines.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/cpu/stringPool.h"
#include "cxxopts/cxxopts.hpp"
#include "platform/windows/graphics/dx12/shaderCompiler.h"
#include "platform/windows/graphics/vk/vkShaderCompiler.h"

const std::string PLUGIN_NAME = "vkShaderCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

inline std::wstring toWstring(const std::string &s) {
  return std::wstring(s.begin(), s.end());
}

bool processArgs(const std::string args,
                 SirEngine::vk::VkShaderArgs &returnArgs) {
  // lets get arguments like they were from commandline
  auto v = splitArgs(args);
  // lets build the options
  cxxopts::Options options("Shader compiler 0.1.0",
                           "Converts a shader in game ready binary blob");
  options.add_options()("t,type", "Type of shader",
                        cxxopts::value<std::string>())(
      "d,debug", "whether to compile debug or release",
      cxxopts::value<std::string>()->implicit_value("1"))(
      "c,compilerArgs", "arguments passed directly to the compiler",
      cxxopts::value<std::string>());
  char **argv = v.argv.get();
  const auto result = options.parse(v.argc, argv);
  if (result.count("type") == 0) {
    SE_CORE_ERROR("{0} : argument type not provided", PLUGIN_NAME);
    return false;
  }
  /*
  if (result.count("entry") == 0) {
    SE_CORE_ERROR("{0} : argument entry point not provided", PLUGIN_NAME);
    return false;
  }

  auto stringEntry = result["entry"].as<std::string>();
  */
  auto stringType = result["type"].as<std::string>();
  returnArgs.debug = result.count("debug");
  returnArgs.type = SirEngine::vk::VkShaderCompiler::getShaderTypeFromName(
      stringType.c_str());
  if (result.count("compilerArgs")) {
    assert(0 && "Compiler args not yet supported for VK compiler");
  }
  return true;
}

void blobToFile(const std::string &assetPath, const std::string &outputPath,
                SirEngine::vk::VkShaderArgs &shaderArgs,
                SirEngine::SpirVBlob blob) {
  // save the file by building a binary request
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::SHADER;
  request.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  VkShaderMapperData mapperData;
  mapperData.shaderFlags =
      SirEngine::vk::VkShaderCompiler::getShaderFlags(shaderArgs);

  std::string entryPoint;
  switch (shaderArgs.type) {
    case SirEngine::SHADER_TYPE::VERTEX: {
      entryPoint = "VS";
      break;
    }
    case SirEngine::SHADER_TYPE::FRAGMENT: {
      entryPoint = "PS";
      break;
    }
    case SirEngine::SHADER_TYPE::COMPUTE: {
      entryPoint = "CS";
      break;
    }
    case SirEngine::SHADER_TYPE::INVALID: {
    }
    default:;
  }
  // what we want to do is to store the data in this way
  //| shader | shader type | shader entry point | shader path | mapper |
  // we need to store enough data for everything
  int totalBulkDataInBytes = static_cast<int>(blob.sizeInByte);
  //+1 is to take into account the termination value
  totalBulkDataInBytes +=
      static_cast<int>(entryPoint.size() * sizeof(char) + 1);
  ;
  totalBulkDataInBytes += static_cast<int>(assetPath.size() + 1);

  // layout the data
  char *dataPtr = reinterpret_cast<char *>(
      SirEngine::globals::FRAME_ALLOCATOR->allocate(totalBulkDataInBytes));
  char *bulkDataPtr = dataPtr;

  // write down the shader in the buffer
  int dataToWriteSizeInByte = static_cast<int>(blob.sizeInByte);
  mapperData.shaderSizeInByte = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, blob.memory, dataToWriteSizeInByte);
  bulkDataPtr += dataToWriteSizeInByte;

  // write down the entry point
  dataToWriteSizeInByte =
      static_cast<int>((entryPoint.size() + 1) * sizeof(char));
  mapperData.entryPointInByte = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, entryPoint.data(), dataToWriteSizeInByte);
  bulkDataPtr += dataToWriteSizeInByte;

  // write down the shader path
  dataToWriteSizeInByte = static_cast<int>(assetPath.size() + 1);
  mapperData.pathSizeInByte = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, assetPath.c_str(), dataToWriteSizeInByte);
  bulkDataPtr += dataToWriteSizeInByte;

  // NOT no compiler args for now
  // write down the compiler args
  // dataToWriteSizeInByte =
  //    static_cast<int>((wcslen(shaderArgs.compilerArgs) + 1) *
  //    sizeof(wchar_t));
  // mapperData.compilerArgsInByte = dataToWriteSizeInByte;
  // memcpy(bulkDataPtr, shaderArgs.compilerArgs, dataToWriteSizeInByte);

  // preparing the binary file write request
  std::filesystem::path inp(assetPath);
  const std::string fileName = inp.stem().string();
  request.outPath = outputPath.c_str();

  request.bulkData = dataPtr;
  request.bulkDataSizeInByte = totalBulkDataInBytes;

  mapperData.type = static_cast<uint32_t>(shaderArgs.type);
  request.mapperData = &mapperData;
  request.mapperDataSizeInByte = sizeof(ShaderMapperData);

  writeBinaryFile(request);
}

bool processShader(const std::string &assetPath, const std::string &outputPath,
                   const std::string &args) {
  // processing plugins args
  SirEngine::vk::VkShaderArgs shaderArgs;

  // checking IO files exits
  bool exits = fileExists(assetPath);
  if (!exits) {
    SE_CORE_ERROR("{0} : could not find path/file {1}", PLUGIN_NAME, assetPath);
  }

  exits = filePathExists(outputPath);
  if (!exits) {
    SE_CORE_ERROR("{0} : could not find path/file {1}", PLUGIN_NAME,
                  outputPath);
  }

  // process arguments, needs to happen after initialization because allocators
  // are used
  bool result = processArgs(args, shaderArgs);
  if (!result) {
    return false;
  }
  if (shaderArgs.type == SirEngine::SHADER_TYPE::INVALID) {
    SE_CORE_ERROR("{0}: Could not parse shader type, type is invalid",
                  PLUGIN_NAME);
  }

  std::string ext = getFileExtension(assetPath);
  SirEngine::SpirVBlob blob;
  if (ext != ".hlsl") {
    // compileSpirV(assetPath, outputPath, shaderArgs, compiler, log);
    SirEngine::vk::VkShaderCompiler compiler;
    std::string log;
    blob = compiler.compileToSpirV(assetPath.c_str(), shaderArgs, &log);

  } else {
    SirEngine::dx12::DXCShaderCompiler compiler;
    SirEngine::dx12::ShaderArgs dxArgs;
    dxArgs.debug = shaderArgs.debug;
    if (shaderArgs.type == SirEngine::SHADER_TYPE::VERTEX) {
      dxArgs.type = L"vs_6_2";
      dxArgs.entryPoint = L"VS";
    }
    if (shaderArgs.type == SirEngine::SHADER_TYPE::FRAGMENT) {
      dxArgs.type = L"ps_6_2";
      dxArgs.entryPoint = L"PS";
    }
    if (shaderArgs.type == SirEngine::SHADER_TYPE::COMPUTE) {
      dxArgs.type = L"cs_6_2";
      dxArgs.entryPoint = L"PS";
    }

    SirEngine::dx12::ShaderCompileResult compileResult =
        compiler.toSpirv(assetPath.c_str(), dxArgs);

    auto size = compileResult.blob->GetBufferSize();
    auto ptr = compileResult.blob->GetBufferPointer();
    void *mem = SirEngine::globals::FRAME_ALLOCATOR->allocate(size);
    memcpy(mem, ptr, size);
    blob = {mem, static_cast<uint32_t>(size)};

    std::vector<unsigned int> spirV;
    spirV.resize(blob.sizeInByte / 4);
    memcpy(spirV.data(), blob.memory, blob.sizeInByte);
    SirEngine::vk::VkShaderCompiler compilerVk;
    std::string res = compilerVk.sprivToGlsl(spirV);
    //std::cout<<res<<std::endl;
  }
  blobToFile(assetPath, outputPath, shaderArgs, blob);

  SE_CORE_INFO("Shader successfully compiled ---> {0}", outputPath);
  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &processShader);
  return true;
}
