#include "shaderCompilerPlugin.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"

#include "SirEngine/binary/binaryFile.h"
#include "resourceCompilerLib/argsUtils.h"

#include <dxcapi.h>
#include <filesystem>

const std::string PLUGIN_NAME = "shaderCompilerPlugin";
const unsigned int versionMajor = 0;
const unsigned int versionMinor = 1;
const unsigned int versionPatch = 0;

LPCWSTR COMPILATION_FLAGS_DEBUG[] = {L"/Zi", L"/Od"};
LPCWSTR COMPILATION_FLAGS[] = {L"/O3"};

struct ShaderArgs {
  bool debug = false;
  std::wstring entryPoint;
  std::wstring type;
};

inline std::wstring toWstring(const std::string &s) {
  return std::wstring(s.begin(), s.end());
}

bool processArgs(const std::string args, ShaderArgs &returnArgs) {
  // lets get arguments like they were from commandline
  auto v = splitArgs(args);
  // lets build the options
  cxxopts::Options options("Shader compiler 0.1.0",
                           "Converts a shader in game ready binary blob");
  options.add_options()("t,type", "Path to the tangent file",
                        cxxopts::value<std::string>())(
      "e,entry", "name of the function to compile",
      cxxopts::value<std::string>())(
      "d,debug", "whether to compile debug or release",
      cxxopts::value<std::string>()->implicit_value("1"));
  char **argv = v.argv.get();
  auto result = options.parse(v.argc, argv);
  if (result.count("type") == 0) {
    SE_CORE_ERROR("{0} : argument type not provided", PLUGIN_NAME);
    return false;
  }
  if (result.count("entry") == 0) {
    SE_CORE_ERROR("{0} : argument entry point not provided", PLUGIN_NAME);
    return false;
  }

  returnArgs.debug = result.count("debug");
  returnArgs.entryPoint = toWstring(result["entry"].as<std::string>());
  returnArgs.type = toWstring(result["type"].as<std::string>());

  return true;
}

bool processShader(const std::string &assetPath, const std::string &outputPath,
                   const std::string &args) {

  // processing plugins args
  std::string tangentsPath = "";
  std::string skinPath = "";
  ShaderArgs shaderArgs;
  bool result = processArgs(args, shaderArgs);
  if (!result) {
    return false;
  }

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

  // lets create an instance of the compiler
  IDxcCompiler *pCompiler;
  DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler),
                    (void **)&pCompiler);

  // creating a blob of data with the content of the shader
  std::wstring wshader{assetPath.begin(), assetPath.end()};
  IDxcOperationResult *pResult;
  // loading shader into shader
  std::ifstream shaderStream(assetPath);
  std::string shaderContent((std::istreambuf_iterator<char>(shaderStream)),
                            std::istreambuf_iterator<char>());
  const char *Program = shaderContent.c_str();
  IDxcLibrary *pLibrary;
  IDxcBlobEncoding *pSource;
  DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary),
                    (void **)&pLibrary);
  pLibrary->CreateBlobWithEncodingFromPinned(Program, shaderContent.size(),
                                             CP_UTF8, &pSource);

  // lets build compiler flags, for now they are simple so we can just switch
  // between two sets based on debug or not
  LPCWSTR *flags =
      shaderArgs.debug ? COMPILATION_FLAGS_DEBUG : COMPILATION_FLAGS;
  int flagsCount = shaderArgs.debug ? _countof(COMPILATION_FLAGS_DEBUG)
                                    : _countof(COMPILATION_FLAGS);

  // kick the compilation
  pCompiler->Compile(pSource,         // program text
                     wshader.c_str(), // file name, mostly for error messages
                     shaderArgs.entryPoint.c_str(), // entry point function
                     shaderArgs.type.c_str(),       // target profile
                     flags,                         // compilation arguments
                     flagsCount, // number of compilation arguments
                     nullptr, 0, // name/value defines and their count
                     nullptr,    // handler for #include directives
                     &pResult);

  // checking whether or not compilation was successiful
  HRESULT hrCompilation;
  pResult->GetStatus(&hrCompilation);

  if (FAILED(hrCompilation)) {

    IDxcBlobEncoding *pPrintBlob, *pPrintBlob16;
    pResult->GetErrorBuffer(&pPrintBlob);

    const std::string errorOut((char *)pPrintBlob->GetBufferPointer(),
                               pPrintBlob->GetBufferSize());
    SE_CORE_ERROR("ERROR_LOG:\n {0}", errorOut);
    pPrintBlob->Release();
  }

  //extract compiled blob of data 
  IDxcBlob *pResultBlob;
  pResult->GetResult(&pResultBlob);

  //save the file by building a binary request
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::SHADER;
  request.version = ((versionMajor << 16) | (versionMinor << 8) | versionPatch);

  // writing binary file
  std::experimental::filesystem::path inp(assetPath);
  const std::string fileName = inp.stem().string().c_str();
  const std::string outFilePath = outputPath;
  request.outPath = outFilePath.c_str();

  // need to merge indices and vertices
  request.bulkData = pResultBlob->GetBufferPointer();
  request.bulkDataSizeInBtye = pResultBlob->GetBufferSize();

  ShaderMapperData mapperData;
  mapperData.shaderType = 0;
  mapperData.shaderSizeInBtye = pResultBlob->GetBufferSize();
  request.mapperData = &mapperData;
  request.mapperDataSizeInByte = sizeof(ShaderMapperData);

  writeBinaryFile(request);

  // release compiler data
  pResult->Release();
  pResultBlob->Release();

  SE_CORE_INFO("Shader successfully compiled ---> {0}", outputPath);
  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &processShader);
  return true;
}
