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

  exits = filePathExists(outputPath);
  if (!exits) {
    SE_CORE_ERROR("[PSO Compiler] : could not find path/file {0}", outputPath);
  }

  // graphics api target
  std::string target;
  processArgs(args, target);

  //std::vector<ResultRoot> blobs;
  //processPSO(assetPath.c_str(), blobs);
  //SE_CORE_INFO("[PSO Compiler: found {0} root signatures", blobs.size());

  /*
  for (auto &subBlobl : blobs) {

    if (subBlobl.blob == nullptr) {
      continue;
    }
    // writing binary file
    BinaryFileWriteRequest request;
    request.fileType = BinaryFileType::RS;
    request.version =
        ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

    std::filesystem::path inp(assetPath);
    const std::string fileName = inp.stem().string();
    const std::string outFilePath =
        getPathName(outputPath) + "/" + subBlobl.name + ".root";
    request.outPath = outFilePath.c_str();

    request.bulkData = subBlobl.blob->GetBufferPointer();
    request.bulkDataSizeInByte = subBlobl.blob->GetBufferSize();

    RootSignatureMappedData mapperData;
    mapperData.type = static_cast<int>(subBlobl.type);
    mapperData.sizeInByte =
  static_cast<uint32_t>(subBlobl.blob->GetBufferSize()); request.mapperData =
  &mapperData; request.mapperDataSizeInByte = sizeof(ModelMapperData);

    writeBinaryFile(request);

    SE_CORE_INFO("Root signature successfully compiled ---> {0}", outputPath);

    // write clean RS for debugging, mostly Intel static analysis
    const std::string outFilePathClean =
        getPathName(outputPath) + "/" + subBlobl.name + ".bin";

    std::ofstream myFile(outFilePathClean, std::ios::out | std::ios::binary);
    myFile.write(static_cast<const char *>(request.bulkData),
  request.bulkDataSizeInByte); myFile.close();
  }
  */
  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &process);
  return true;
}
