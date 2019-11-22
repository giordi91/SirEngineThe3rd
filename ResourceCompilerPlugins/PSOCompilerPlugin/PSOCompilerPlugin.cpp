#include "PSOCompilerPlugin.h"
#include "PSOProcess.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"

#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"

const std::string PLUGIN_NAME = "PSOCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

void processArgs(const std::string args, std::string &target) {
  // lets get arguments like they were from commandline
  auto v = splitArgs(args);
  // lets build the options
  cxxopts::Options options("PSO Compiler", "Compilers a Pipeline state object");
  options.add_options()("t,target",
                        "which API to target, valid options are DX or VK",
                        cxxopts::value<std::string>());

  char **argv = v.argv.get();
  const cxxopts::ParseResult result = options.parse(v.argc, argv);

  if (result.count("target")) {
    target = result["target"].as<std::string>();
  } else {
    target = "DX";
  }
}

bool process(const std::string &assetPath, const std::string &outputPath,
             const std::string &args) {

  // checking IO files exits
  bool exits = fileExists(assetPath);
  if (!exits) {
    SE_CORE_ERROR("[PSO Compiler] : could not find path/file {0}", assetPath);
  }

  //exits = filePathExists(outputPath);
  //if (!exits) {
  //  SE_CORE_ERROR("[PSO Compiler] : could not find path/file {0}", outputPath);
  //}

  // graphics api target
  std::string target;
  processArgs(args, target);

  SirEngine::dx12::PSOCompileResult result = processPSO(assetPath.c_str());

  // writing binary file
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::RS;
  request.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  std::filesystem::path inp(assetPath);
  const std::string fileName = inp.stem().string();
  const std::string outFilePath =outputPath;
  request.outPath = outFilePath.c_str();

  ID3DBlob *blob = nullptr;
  result.pso->GetCachedBlob(&blob);

  //+1 is \0 for string
  std::vector<char> bulkData(blob->GetBufferSize() +
                             sizeof(char) * assetPath.size() + 1);
  char *bulkDataPtr = bulkData.data();

  memcpy(bulkDataPtr, blob->GetBufferPointer(), blob->GetBufferSize());
  memcpy(bulkDataPtr + blob->GetBufferSize(), assetPath.c_str(),
         assetPath.size() + 1);

  request.bulkData = bulkDataPtr;
  request.bulkDataSizeInByte = bulkData.size();

  PSOMappedData mapperData;
  mapperData.psoSizeInByte = blob->GetBufferSize();
  mapperData.psoNameSizeInByte = assetPath.size() * sizeof(char);
  request.mapperData = &mapperData;
  request.mapperDataSizeInByte = sizeof(PSOMappedData);
  request.fileType = BinaryFileType::PSO;

  writeBinaryFile(request);

  SE_CORE_INFO("PSO successfully compiled ---> {0}", outFilePath);

  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &process);
  return true;
}
