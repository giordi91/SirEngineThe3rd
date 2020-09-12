#include "PSOCompilerPlugin.h"

#include "PSOProcess.h"
#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/materialManager.h"
#include "cxxopts/cxxopts.hpp"

const std::string PLUGIN_NAME = "PSOCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

void processArgs(const std::string args, std::string &target,
                 std::string &shaderPath, bool &processFolder) {
  // lets get arguments like they were from commandline
  auto v = splitArgs(args);
  // lets build the options
  cxxopts::Options options("PSO Compiler", "Compilers a Pipeline state object");
  options.add_options()("t,target",
                        "which API to target, valid options are DX or VK",
                        cxxopts::value<std::string>())(
      "s,shaderPath",
      "base path for shaders, before the folder structure start, like "
      "common,rasterization etc",
      cxxopts::value<std::string>())(
      "a,allInFolder", "compile all files in the given folder path",
      cxxopts::value<std::string>()->implicit_value("1"));

  char **argv = v.argv.get();
  const cxxopts::ParseResult result = options.parse(v.argc, argv);

  if (result.count("target")) {
    target = result["target"].as<std::string>();
  } else {
    target = "DX12";
  }
  if (result.count("shaderPath")) {
    shaderPath = result["shaderPath"].as<std::string>();
  } else {
    shaderPath = "";
  }
  processFolder = result.count("allInFolder") > 0;
}

bool compileAndSavePSO(const std::string &assetPath,
                       const std::string &outputPath,
                       const std::string &shaderPath) {
  // for now we do not support pso compilation BUT we can export all the
  // metadata stuff
  SirEngine::MaterialMetadata metadata =
      SirEngine::extractMetadata(assetPath.c_str());
  // need to serialize metadata
  uint32_t structSize =
      (metadata.objectResourceCount + metadata.frameResourceCount +
       metadata.passResourceCount) *
      sizeof(SirEngine::MaterialResource);
  uint32_t stringSizes = 0;
  for (uint32_t i = 0; i < metadata.objectResourceCount; ++i) {
    stringSizes += (strlen(metadata.objectResources[i].name) + 1);
  }
  for (uint32_t i = 0; i < metadata.frameResourceCount; ++i) {
    stringSizes += (strlen(metadata.frameResources[i].name) + 1);
  }
  for (uint32_t i = 0; i < metadata.passResourceCount; ++i) {
    stringSizes += (strlen(metadata.passResources[i].name) + 1);
  }
  uint32_t totalSize = structSize += stringSizes;
  std::vector<uint8_t> data;
  data.resize(totalSize);

  uint32_t counter = 0;
  for (uint32_t i = 0; i < metadata.objectResourceCount; ++i) {
    size_t len = strlen(metadata.objectResources[i].name) + 1;
    memcpy(data.data() + counter, metadata.objectResources[i].name, len);
    // patching the pointer with an offset pointer from the start
    metadata.objectResources[i].name =
        reinterpret_cast<const char *>((data.data() + counter) - data.data());
    counter += len;
  }
  for (uint32_t i = 0; i < metadata.frameResourceCount; ++i) {
    size_t len = strlen(metadata.frameResources[i].name) + 1;
    memcpy(data.data() + counter, metadata.frameResources[i].name, len);
    // patching the pointer with an offset pointer from the start
    metadata.frameResources[i].name =
        reinterpret_cast<const char *>((data.data() + counter) - data.data());
    counter += len;
  }
  for (uint32_t i = 0; i < metadata.passResourceCount; ++i) {
    size_t len = strlen(metadata.passResources[i].name) + 1;
    memcpy(data.data() + counter, metadata.passResources[i].name, len);
    // patching the pointer with an offset pointer from the start
    metadata.passResources[i].name =
        reinterpret_cast<const char *>((data.data() + counter) - data.data());
    counter += len;
  }
  assert(counter == stringSizes);

  MaterialMappedData mapped{};
  // we copied the strings and now can copy the array data
  uint32_t size =
      metadata.objectResourceCount * sizeof(SirEngine::MaterialResource);
  memcpy(data.data() + counter, metadata.objectResources, size);
  mapped.objectResourceDataOffset = counter;
  counter += size;

  size = metadata.frameResourceCount * sizeof(SirEngine::MaterialResource);
  memcpy(data.data() + counter, metadata.frameResources, size);
  mapped.frameResourceDataOffset = counter;
  counter += size;

  size = metadata.passResourceCount * sizeof(SirEngine::MaterialResource);
  memcpy(data.data() + counter, metadata.passResources, size);
  mapped.passResourceDataOffset = counter;
  counter += size;

  assert(counter == totalSize);
  mapped.objectResourceCount = metadata.objectResourceCount;
  mapped.frameResourceCount = metadata.frameResourceCount;
  mapped.passResourceCount = metadata.passResourceCount;

  // writing the data
  // writing binary file
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::MATERIAL_METADATA;
  request.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  request.bulkData = data.data();
  request.bulkDataSizeInByte = totalSize;

  request.mapperData = &mapped;
  request.mapperDataSizeInByte = sizeof(PSOMappedData);
  request.outPath = outputPath.c_str();

  writeBinaryFile(request);

  SE_CORE_INFO("PSO Metadata successfully compiled ---> {0}", outputPath);
  return true;

  /*
SirEngine::dx12::PSOCompileResult result =
    processPSO(assetPath.c_str(), shaderPath.c_str());

// writing binary file
BinaryFileWriteRequest request;
request.fileType = BinaryFileType::RS;
request.version =
    ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

std::filesystem::path inp(assetPath);
const std::string fileName = inp.stem().string();
const std::string outFilePath = outputPath;
request.outPath = outFilePath.c_str();

ID3DBlob *blob = nullptr;
result.pso->GetCachedBlob(&blob);

//+1 is \0 for string
int blobSize = blob->GetBufferSize();
int totalSize = blobSize + sizeof(char) * assetPath.size() + 1;
int graphicsSize = sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC);
int computeSize = sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC);
int descReservedSize =
    (graphicsSize > computeSize ? graphicsSize : computeSize);
totalSize += descReservedSize;

//+1 is \0 for string
int vsNameSize = result.VSName == nullptr ? 0 : strlen(result.VSName) + 1;
int psNameSize = result.PSName == nullptr ? 0 : strlen(result.PSName) + 1;
int csNameSize = result.CSName == nullptr ? 0 : strlen(result.CSName) + 1;

totalSize += (vsNameSize + psNameSize + csNameSize);

// input layout and root signature
int inputLayoutNameSize =
    result.inputLayout == nullptr ? 0 : strlen(result.inputLayout) + 1;
int rootNameSize =
    result.rootSignature == nullptr ? 0 : strlen(result.rootSignature) + 1;
totalSize += (inputLayoutNameSize + rootNameSize);

std::vector<char> bulkData(totalSize);
char *bulkDataPtr = bulkData.data();

// first we copy the actual compiled blob
int ptrPos = 0;
memcpy(bulkDataPtr, blob->GetBufferPointer(), blob->GetBufferSize());
ptrPos += blob->GetBufferSize();

// second we copy the structure description
if (result.psoType == SirEngine::PSO_TYPE::RASTER) {
  memcpy(bulkDataPtr + ptrPos, result.graphicDesc, graphicsSize);
} else {
  memcpy(bulkDataPtr + ptrPos, result.computeDesc, computeSize);
}
auto tmp = reinterpret_cast<D3D12_GRAPHICS_PIPELINE_STATE_DESC *>(
    bulkDataPtr + ptrPos);
ptrPos += descReservedSize;
// next we copy the actual pso path
memcpy(bulkDataPtr + ptrPos, assetPath.c_str(), assetPath.size() + 1);
ptrPos += assetPath.size() + 1;
// next the shaders
if (vsNameSize != 0) {
  memcpy(bulkDataPtr + ptrPos, result.VSName, vsNameSize);
  ptrPos += vsNameSize;
}
if (psNameSize != 0) {
  memcpy(bulkDataPtr + ptrPos, result.PSName, psNameSize);
  ptrPos += psNameSize;
}
if (csNameSize != 0) {
  memcpy(bulkDataPtr + ptrPos, result.CSName, csNameSize);
  ptrPos += csNameSize;
}

// storing input layout
// TODO can i do memcpy with zero size?
if (inputLayoutNameSize != 0) {
  memcpy(bulkDataPtr + ptrPos, result.inputLayout, inputLayoutNameSize);
  ptrPos += inputLayoutNameSize;
}
// finally root signature
if (rootNameSize != 0) {
  memcpy(bulkDataPtr + ptrPos, result.rootSignature, rootNameSize);
  ptrPos += rootNameSize;
}

request.bulkData = bulkDataPtr;
request.bulkDataSizeInByte = bulkData.size();

PSOMappedData mapperData{};
mapperData.psoSizeInByte = blob->GetBufferSize();
mapperData.psoNameSizeInByte = assetPath.size() * sizeof(char) + 1;
mapperData.psoDescSizeInByte = descReservedSize;
mapperData.psoType = static_cast<int>(result.psoType);
mapperData.vsShaderNameSize = vsNameSize;
mapperData.psShaderNameSize = psNameSize;
mapperData.csShaderNameSize = csNameSize;
mapperData.inputLayoutSize = inputLayoutNameSize;
mapperData.rootSignatureSize = rootNameSize;
mapperData.topologyType = static_cast<int>(result.topologyType);

request.mapperData = &mapperData;
request.mapperDataSizeInByte = sizeof(PSOMappedData);
request.fileType = BinaryFileType::PSO;

writeBinaryFile(request);

SE_CORE_INFO("PSO successfully compiled ---> {0}", outFilePath);
return true;
*/
}

bool compileDx12(const std::string &assetPath, const std::string &outputPath,
                 std::string target, std::string shaderPath) {
  // get files in folder and process them
  const std::string graphicsDirectory = "/" + target + "/";

  std::vector<std::string> filePaths;
  listFilesInFolder(assetPath.c_str(), filePaths, "json");
  for (auto &path : filePaths) {
    const std::string fileName = getFileName(path);
    const std::string currOutputPath =
        outputPath + graphicsDirectory + fileName + ".metadata";
    compileAndSavePSO(path, currOutputPath, shaderPath);
  }
  return true;
}

bool compileVK(const std::string &assetPath, const std::string &outputPath,
               const std::string &target, const std::string &shaderPath) {
  // for now we do not support pso compilation BUT we can export all the
  // metadata stuff
  SirEngine::MaterialMetadata metadata =
      SirEngine::extractMetadata(assetPath.c_str());
  // need to serialize metadata
  uint32_t structSize =
      (metadata.objectResourceCount + metadata.frameResourceCount +
       metadata.passResourceCount) *
      sizeof(SirEngine::MaterialResource);
  uint32_t stringSizes = 0;
  for (uint32_t i = 0; i < metadata.objectResourceCount; ++i) {
    stringSizes += (strlen(metadata.objectResources[i].name) + 1);
  }
  for (uint32_t i = 0; i < metadata.frameResourceCount; ++i) {
    stringSizes += (strlen(metadata.frameResources[i].name) + 1);
  }
  for (uint32_t i = 0; i < metadata.passResourceCount; ++i) {
    stringSizes += (strlen(metadata.passResources[i].name) + 1);
  }
  uint32_t totalSize = structSize += stringSizes;
  std::vector<uint8_t> data;
  data.resize(totalSize);

  uint32_t counter = 0;
  for (uint32_t i = 0; i < metadata.objectResourceCount; ++i) {
    size_t len = strlen(metadata.objectResources[i].name) + 1;
    memcpy(data.data() + counter, metadata.objectResources[i].name, len);
    // patching the pointer with an offset pointer from the start
    metadata.objectResources[i].name =
        reinterpret_cast<const char *>((data.data() + counter) - data.data());
    counter += len;
  }
  for (uint32_t i = 0; i < metadata.frameResourceCount; ++i) {
    size_t len = strlen(metadata.frameResources[i].name) + 1;
    memcpy(data.data() + counter, metadata.frameResources[i].name, len);
    // patching the pointer with an offset pointer from the start
    metadata.frameResources[i].name =
        reinterpret_cast<const char *>((data.data() + counter) - data.data());
    counter += len;
  }
  for (uint32_t i = 0; i < metadata.passResourceCount; ++i) {
    size_t len = strlen(metadata.passResources[i].name) + 1;
    memcpy(data.data() + counter, metadata.passResources[i].name, len);
    // patching the pointer with an offset pointer from the start
    metadata.passResources[i].name =
        reinterpret_cast<const char *>((data.data() + counter) - data.data());
    counter += len;
  }
  assert(counter == stringSizes);

  MaterialMappedData mapped{};
  // we copied the strings and now can copy the array data
  uint32_t size =
      metadata.objectResourceCount * sizeof(SirEngine::MaterialResource);
  memcpy(data.data() + counter, metadata.objectResources, size);
  mapped.objectResourceDataOffset = counter;
  counter += size;

  size = metadata.frameResourceCount * sizeof(SirEngine::MaterialResource);
  memcpy(data.data() + counter, metadata.frameResources, size);
  mapped.frameResourceDataOffset = counter;
  counter += size;

  size = metadata.passResourceCount * sizeof(SirEngine::MaterialResource);
  memcpy(data.data() + counter, metadata.passResources, size);
  mapped.passResourceDataOffset = counter;
  counter += size;

  assert(counter == totalSize);
  mapped.objectResourceCount = metadata.objectResourceCount;
  mapped.frameResourceCount = metadata.frameResourceCount;
  mapped.passResourceCount = metadata.passResourceCount;

  // writing the data
  // writing binary file
  BinaryFileWriteRequest request;
  request.fileType = BinaryFileType::MATERIAL_METADATA;
  request.version =
      ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH);

  request.bulkData = data.data();
  request.bulkDataSizeInByte = totalSize;

  request.mapperData = &mapped;
  request.mapperDataSizeInByte = sizeof(PSOMappedData);
  request.outPath = outputPath.c_str();

  writeBinaryFile(request);

  SE_CORE_INFO("PSO Metadata successfully compiled ---> {0}", outputPath);
  return true;
}

bool compileAllVK(const std::string &assetPath, const std::string &outputPath,
                  const std::string &target, const std::string &shaderPath) {
  // get files in folder and process them
  const std::string graphicsDirectory = "/" + target + "/";

  std::vector<std::string> filePaths;
  listFilesInFolder(assetPath.c_str(), filePaths, "json");
  for (auto &path : filePaths) {
    const std::string fileName = getFileName(path);
    const std::string currOutputPath =
        outputPath + graphicsDirectory + fileName + ".pso";
    compileVK(path, currOutputPath, target, shaderPath);
  }
  return true;
}

bool compileAllDx12(const std::string &assetPath, const std::string &outputPath,
                    const std::string &target, const std::string &shaderPath) {
  // get files in folder and process them
  const std::string graphicsDirectory = "/" + target + "/";

  std::vector<std::string> filePaths;
  listFilesInFolder(assetPath.c_str(), filePaths, "json");
  for (auto &path : filePaths) {
    const std::string fileName = getFileName(path);
    const std::string currOutputPath =
        outputPath + graphicsDirectory + fileName + ".metadata";
    compileAndSavePSO(path, currOutputPath, shaderPath);
  }
  return true;
}

bool process(const std::string &assetPath, const std::string &outputPath,
             const std::string &args) {
  // checking IO files exits
  bool exits = fileExists(assetPath);
  if (!exits) {
    SE_CORE_ERROR("[PSO Compiler] : could not find path/file {0}", assetPath);
  }

  std::string target;
  std::string shaderPath;
  bool processFolder = false;
  processArgs(args, target, shaderPath, processFolder);

  if (processFolder && !isPathDirectory(assetPath)) {
    SE_CORE_ERROR(
        "Requested to compile all PSO in path, but provided path is "
        "not a directory");
  }
  if (shaderPath.empty()) {
    SE_CORE_ERROR("Shader path not provided, cannot compile PSO");
    return false;
  }

  if (!processFolder) {
    // compileAndSavePSO(assetPath, outputPath, shaderPath);
    // return true;
    if (target == "DX12") {
      assert(0);
      return compileDx12(assetPath, outputPath, target, shaderPath);
    } else {
      return compileVK(assetPath, outputPath, target, shaderPath);
    }
  }

  if (target == "DX12") {
    return compileAllDx12(assetPath, outputPath, target, shaderPath);
  } else {
    return compileAllVK(assetPath, outputPath, target, shaderPath);
  }

  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &process);
  return true;
}
