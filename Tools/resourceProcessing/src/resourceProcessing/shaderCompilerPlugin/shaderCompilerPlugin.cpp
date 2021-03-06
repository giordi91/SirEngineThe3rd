#include "shaderCompilerPlugin.h"

#include <d3dcommon.h>

#include <filesystem>

#include "SirEngine/io/argsUtils.h"
#include "SirEngine/io/binaryFile.h"
#include "SirEngine/io/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/cpu/stringPool.h"
#include "cxxopts/cxxopts.hpp"
#include "platform/windows/graphics/dx12/shaderCompiler.h"

const std::string PLUGIN_NAME = "shaderCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

inline std::wstring toWstring(const std::string &s) {
  return std::wstring(s.begin(), s.end());
}

bool processArgs(const std::string args,
                 SirEngine::dx12::ShaderArgs &returnArgs) {
  // lets get arguments like they were from commandline
  auto v = splitArgs(args);
  // lets build the options
  cxxopts::Options options("Shader compiler 0.1.0",
                           "Converts a shader in game ready binary blob");
  options.add_options()("t,type", "Type of shader",
                        cxxopts::value<std::string>())(
      "e,entry", "name of the function to compile",
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
  if (result.count("entry") == 0) {
    SE_CORE_ERROR("{0} : argument entry point not provided", PLUGIN_NAME);
    return false;
  }

  auto stringEntry = result["entry"].as<std::string>();
  auto stringType = result["type"].as<std::string>();
  returnArgs.debug = result.count("debug");
  // TODO can I do anything about this const cast?
  returnArgs.entryPoint = const_cast<wchar_t *>(
      SirEngine::globals::STRING_POOL->convertWide(stringEntry.c_str()));
  returnArgs.type = const_cast<wchar_t *>(
      SirEngine::globals::STRING_POOL->convertWide(stringType.c_str()));
  if (result.count("compilerArgs")) {
    const std::string cargs = result["compilerArgs"].as<std::string>();
    std::string strippedCargs = cargs.substr(1, cargs.length() - 2);
    returnArgs.compilerArgs = const_cast<wchar_t *>(
        SirEngine::globals::STRING_POOL->convertWide(strippedCargs.c_str()));
    splitCompilerArgs(strippedCargs, returnArgs.splitCompilerArgsPointers);

  } else {
    returnArgs.compilerArgs = L"";
  }

  return true;
}

bool processShader(const std::string &assetPath, const std::string &outputPath,
                   const std::string &args) {
  // processing plugins args
  SirEngine::dx12::ShaderArgs shaderArgs;

  // checking IO files exits
  bool exits = SirEngine::fileExists(assetPath);
  if (!exits) {
    SE_CORE_ERROR("{0} : could not find path/file {1}", PLUGIN_NAME, assetPath);
  }

  exits = SirEngine::filePathExists(outputPath);
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

  SirEngine::dx12::DXCShaderCompiler compiler;
  SirEngine::dx12::ShaderCompileResult compileResult =
      compiler.compileShader(assetPath.c_str(), shaderArgs);
  if (compileResult.blob == nullptr) {
    SE_CORE_ERROR("failed to compile shader {0}", assetPath);
    return false;
  }

  // save the file by building a binary request
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::SHADER;
  request.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  ShaderMapperData mapperData;
  mapperData.shaderFlags = compiler.getShaderFlags(shaderArgs);

  // what we want to do is to store the data in this way
  //| shader | shader type | shader entry point | shader path | mapper |
  // we need to store enough data for everything
  int totalBulkDataInBytes =
      static_cast<int>(compileResult.blob->GetBufferSize());
  //+1 is to take into account the termination value
  totalBulkDataInBytes +=
      static_cast<int>((wcslen(shaderArgs.type) + 1) * sizeof(wchar_t));
  totalBulkDataInBytes +=
      static_cast<int>((wcslen(shaderArgs.entryPoint) + 1) * sizeof(wchar_t));
  totalBulkDataInBytes += static_cast<int>(assetPath.size() + 1);
  totalBulkDataInBytes +=
      static_cast<int>((wcslen(shaderArgs.compilerArgs) + 1) * sizeof(wchar_t));

  // layout the data

  char *dataPtr = reinterpret_cast<char *>(
      SirEngine::globals::FRAME_ALLOCATOR->allocate(totalBulkDataInBytes));
  char *bulkDataPtr = dataPtr;

  // write down the shader in the buffer
  int dataToWriteSizeInByte =
      static_cast<int>(compileResult.blob->GetBufferSize());
  mapperData.shaderSizeInByte = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, compileResult.blob->GetBufferPointer(),
         dataToWriteSizeInByte);
  bulkDataPtr += dataToWriteSizeInByte;

  // write down the type
  dataToWriteSizeInByte =
      static_cast<int>((wcslen(shaderArgs.type) + 1) * sizeof(wchar_t));
  mapperData.typeSizeInByte = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, shaderArgs.type, dataToWriteSizeInByte);
  bulkDataPtr += dataToWriteSizeInByte;

  // write down the entry point
  dataToWriteSizeInByte =
      static_cast<int>((wcslen(shaderArgs.entryPoint) + 1) * sizeof(wchar_t));
  mapperData.entryPointInByte = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, shaderArgs.entryPoint, dataToWriteSizeInByte);
  bulkDataPtr += dataToWriteSizeInByte;

  // write down the shader path
  dataToWriteSizeInByte = static_cast<int>(assetPath.size() + 1);
  mapperData.pathSizeInByte = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, assetPath.c_str(), dataToWriteSizeInByte);
  bulkDataPtr += dataToWriteSizeInByte;

  // write down the compiler args
  dataToWriteSizeInByte =
      static_cast<int>((wcslen(shaderArgs.compilerArgs) + 1) * sizeof(wchar_t));
  mapperData.compilerArgsInByte = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, shaderArgs.compilerArgs, dataToWriteSizeInByte);

  // preparing the binary file write request
  std::filesystem::path inp(assetPath);
  const std::string fileName = inp.stem().string();
  request.outPath = outputPath.c_str();

  request.bulkData = dataPtr;
  request.bulkDataSizeInByte = totalBulkDataInBytes;

  mapperData.shaderSizeInByte =
      static_cast<uint32_t>(compileResult.blob->GetBufferSize());
  request.mapperData = &mapperData;
  request.mapperDataSizeInByte = sizeof(ShaderMapperData);

  writeBinaryFile(request);

  // release compiler data
  // no need to release the const char* log if there, because is temp allocated
  // anyway
  compileResult.blob->Release();

  SE_CORE_INFO("Shader successfully compiled ---> {0}", outputPath);
  return true;
}

