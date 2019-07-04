#include "shaderCompilerPlugin.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"

#include "SirEngine/binary/binaryFile.h"
#include "platform/windows/graphics/dx12/shaderCompiler.h"
#include "resourceCompilerLib/argsUtils.h"

#include <dxcapi.h>
#include <filesystem>

#include <d3dcompiler.h>

const std::string PLUGIN_NAME = "shaderCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

LPCWSTR COMPILATION_FLAGS_DEBUG[] = {L"/Zi", L"/Od"};
LPCWSTR COMPILATION_FLAGS[] = {L"/O3"};

enum SHADER_FLAGS { DEBUG = 1, AMD_INSTRINSICS = 2, NVIDIA_INSTRINSICS = 4 };

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

unsigned int getShaderFlags(const SirEngine::dx12::ShaderArgs &args) {
  unsigned int flags = 0;
  flags |= (args.debug ? SHADER_FLAGS::DEBUG : 0);
  flags |= (args.isAMD ? SHADER_FLAGS::AMD_INSTRINSICS : 0);
  flags |= (args.isNVidia ? SHADER_FLAGS::NVIDIA_INSTRINSICS : 0);

  return flags;
}

bool processShader(const std::string &assetPath, const std::string &outputPath,
                   const std::string &args) {

  // processing plugins args
  std::string tangentsPath = "";
  std::string skinPath = "";
  SirEngine::dx12::ShaderArgs shaderArgs;
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


  SirEngine::dx12::DXCShaderCompiler compiler;

  ID3DBlob* blob = compiler.compilerShader(assetPath,shaderArgs);

  /*

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

  // create a standard include
  IDxcIncludeHandler *includeHandle = nullptr;
  pLibrary->CreateIncludeHandler(&includeHandle);

  // kick the compilation
  pCompiler->Compile(pSource,         // program text
                     wshader.c_str(), // file name, mostly for error messages
                     shaderArgs.entryPoint.c_str(), // entry point function
                     shaderArgs.type.c_str(),       // target profile
                     flags,                         // compilation arguments
                     flagsCount,    // number of compilation arguments
                     nullptr, 0,    // name/value defines and their count
                     includeHandle, // handler for #include directives
                     &pResult);

  // checking whether or not compilation was successiful
  HRESULT hrCompilation;
  pResult->GetStatus(&hrCompilation);

  if (FAILED(hrCompilation)) {

    IDxcBlobEncoding *pPrintBlob;
    pResult->GetErrorBuffer(&pPrintBlob);

    const std::string errorOut(
        static_cast<char *>(pPrintBlob->GetBufferPointer()),
        pPrintBlob->GetBufferSize());
    SE_CORE_ERROR("ERROR_LOG:\n {0}", errorOut);
    pPrintBlob->Release();
  }

  // extract compiled blob of data
  IDxcBlob *pResultBlob;
  pResult->GetResult(&pResultBlob);

  ID3DBlob *blob;
  HRESULT hr = D3DCreateBlob(pResultBlob->GetBufferSize(), &blob);
  assert(SUCCEEDED(hr) && "could not create shader blob");
  memcpy(blob->GetBufferPointer(),pResultBlob->GetBufferPointer(),pResultBlob->GetBufferSize());
  */

  // save the file by building a binary request
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::SHADER;
  request.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  ShaderMapperData mapperData;
  mapperData.shaderFlags = getShaderFlags(shaderArgs);

  // what we want to do is to store the data in this way
  //| shader | shader type | shader entry point | shader path | mapper |
  // we need to store enough data for everything
  int totalBulkDataInBytes = blob->GetBufferSize();
  //+1 is to take into account the termination value
  totalBulkDataInBytes += (shaderArgs.type.size() + 1) * sizeof(wchar_t);
  totalBulkDataInBytes += (shaderArgs.entryPoint.size() + 1) * sizeof(wchar_t);
  totalBulkDataInBytes += (assetPath.size() + 1);

  // layout the data
  std::vector<char> bulkData(totalBulkDataInBytes);
  char *bulkDataPtr = bulkData.data();

  // write down the shader in the buffer
  int dataToWriteSizeInByte = blob->GetBufferSize();
  mapperData.shaderSizeInBtye = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, blob->GetBufferPointer(), dataToWriteSizeInByte);
  bulkDataPtr += dataToWriteSizeInByte;

  // write down the type
  dataToWriteSizeInByte = (shaderArgs.type.size() + 1) * sizeof(wchar_t);
  mapperData.typeSizeInByte = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, shaderArgs.type.c_str(), dataToWriteSizeInByte);
  bulkDataPtr += dataToWriteSizeInByte;

  // write down the entry point
  dataToWriteSizeInByte = (shaderArgs.entryPoint.size() + 1) * sizeof(wchar_t);
  mapperData.entryPointInByte = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, shaderArgs.entryPoint.c_str(), dataToWriteSizeInByte);
  bulkDataPtr += dataToWriteSizeInByte;

  // write down the shader path
  dataToWriteSizeInByte = (assetPath.size() + 1);
  mapperData.pathSizeInBtype = dataToWriteSizeInByte;
  memcpy(bulkDataPtr, assetPath.c_str(), dataToWriteSizeInByte);

  // preparing the binary file write request
  std::experimental::filesystem::path inp(assetPath);
  const std::string fileName = inp.stem().string().c_str();
  const std::string outFilePath = outputPath;
  request.outPath = outFilePath.c_str();

  request.bulkData = bulkData.data();
  request.bulkDataSizeInBtye = bulkData.size();

  mapperData.shaderSizeInBtye = blob->GetBufferSize();
  request.mapperData = &mapperData;
  request.mapperDataSizeInByte = sizeof(ShaderMapperData);

  writeBinaryFile(request);

  // release compiler data
  //pResult->Release();
  //pResultBlob->Release();
  blob->Release();

  SE_CORE_INFO("Shader successfully compiled ---> {0}", outputPath);
  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &processShader);
  return true;
}
