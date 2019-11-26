#include "rootSignatureCompilerPlugin.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"
#include "platform/windows/graphics/dx12/rootSignatureCompile.h"

#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"

const std::string PLUGIN_NAME = "rootSignatureCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

bool processRoot(const std::string &assetPath, const std::string &outputPath,
                 const std::string &) {

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

  ID3D10Blob *blob;
  auto result =
      SirEngine::dx12::processSignatureFileToBlob(assetPath.c_str(), &blob);

  if (blob == nullptr) {
	SE_CORE_ERROR("Could not compiler root signature {0}",assetPath);
	return false;
  }
  // writing binary file
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::RS;
  request.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  std::filesystem::path inp(assetPath);
  const std::string fileName = inp.stem().string();
  const std::string outFilePath =
      getPathName(outputPath) + "/" + result.name + ".root";
  request.outPath = outFilePath.c_str();

  request.bulkData = blob->GetBufferPointer();
  request.bulkDataSizeInByte = blob->GetBufferSize();

  RootSignatureMappedData mapperData;
  mapperData.type = static_cast<int>(result.type);
  mapperData.sizeInByte = static_cast<uint32_t>(blob->GetBufferSize());
  request.mapperData = &mapperData;
  request.mapperDataSizeInByte = sizeof(RootSignatureMappedData);

  writeBinaryFile(request);

  SE_CORE_INFO("Root signature successfully compiled ---> {0}", outputPath);

  // write clean RS for debugging, mostly Intel static analysis
  const std::string outFilePathClean =
      getPathName(outputPath) + "/" + result.name + ".bin";

  std::ofstream myFile(outFilePathClean, std::ios::out | std::ios::binary);
  myFile.write(static_cast<const char *>(request.bulkData),
               request.bulkDataSizeInByte);
  myFile.close();
  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &processRoot);
  return true;
}
