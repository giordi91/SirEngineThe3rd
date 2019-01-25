#include "rootSignatureCompilerPlugin.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"
#include "rootProcess.h"

#include "SirEngine/binary/binaryFile.h"
#include "resourceCompilerLib/argsUtils.h"

const std::string PLUGIN_NAME = "rootSignatureCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

bool processRoot(const std::string &assetPath, const std::string &outputPath,
                 const std::string &args) {

  // checking IO files exits
  bool exits = fileExists(assetPath);
  if (!exits) {
    SE_CORE_ERROR("[Root Signature Compiler] : could not find path/file {0}",
                  assetPath);
  }

  exits = filePathExists(outputPath);
  if (!exits) {
    SE_CORE_ERROR("[Root Signature Compiler] : could not find path/file {0}",
                  outputPath);
  }

  std::vector<ResultRoot> blobs;
  processSignatureFile(assetPath.c_str(), blobs);
  SE_CORE_INFO("[Root Signature Compiler: found {0} root signatures",
               blobs.size());

  for (auto &subBlobl : blobs) {

    if (subBlobl.blob == nullptr) {
      continue;
    }
    // writing binary file
    BinaryFileWriteRequest request;
    request.fileType = BinaryFileType::RS;
    request.version =
        ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

    std::experimental::filesystem::path inp(assetPath);
    const std::string fileName = inp.stem().string();
    const std::string outFilePath =
        getPathName(outputPath) + "/" + subBlobl.name + ".root";
    request.outPath = outFilePath.c_str();

    request.bulkData = subBlobl.blob->GetBufferPointer();
    request.bulkDataSizeInBtye = subBlobl.blob->GetBufferSize();

    RootSignatureMappedData mapperData;
    mapperData.type = static_cast<int>(subBlobl.type);
    mapperData.sizeInByte = subBlobl.blob->GetBufferSize();
    request.mapperData = &mapperData;
    request.mapperDataSizeInByte = sizeof(ModelMapperData);

    writeBinaryFile(request);

    SE_CORE_INFO("Root signature successfully compiled ---> {0}", outputPath);
  }
  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &processRoot);
  return true;
}
