#include "PSOCompilerPlugin.h"

#include "PSOProcess.h"
#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
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

/*
bool compileAndSavePSO(const std::string &assetPath,
                     const std::string &outputPath,
                     const std::string &shaderPath) {
  //old code to compile a PSO we will need it in the future
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
return true;
}
*/

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
    assert(0);
    // compileAndSavePSO(path, currOutputPath, shaderPath);
  }
  return true;
}

bool compileVK(const std::string &assetPath, const std::string &outputPath) {
  // for now we do not support pso compilation BUT we can export all the
  // metadata stuff
  SirEngine::graphics::MaterialMetadata metadata =
      SirEngine::graphics::extractMetadataFromPSO(assetPath.c_str());
  // need to serialize metadata
  size_t structSize =
      (metadata.objectResourceCount + metadata.frameResourceCount +
       metadata.passResourceCount) *
      sizeof(SirEngine::graphics::MaterialResource);
  size_t uniformsMetaSize = 0;

  // we only store the values for the object resource count
  // pass data and frame data are handled by the engine directly
  for (uint32_t i = 0; i < metadata.objectResourceCount; ++i) {
    if (metadata.objectResources[i].type ==
        SirEngine::graphics::MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER) {
      uniformsMetaSize +=
          sizeof(SirEngine::graphics::MaterialMetadataStructMember) *
          metadata.objectResources[i].extension.uniform.membersCount;
    }
  }

  uint32_t totalSize = static_cast<uint32_t>(structSize + uniformsMetaSize);
  std::vector<uint8_t> data;
  data.resize(totalSize);

  size_t counter = 0;

  for (uint32_t i = 0; i < metadata.objectResourceCount; ++i) {
    // if the object is a constant buffer it comes with a uniform metadata
    // we have have to write on disk and patch the pointer
    if (metadata.objectResources[i].type ==
        SirEngine::graphics::MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER) {
      auto size = sizeof(SirEngine::graphics::MaterialMetadataStructMember) *
                  metadata.objectResources[i].extension.uniform.membersCount;
      memcpy(data.data() + counter,
             metadata.objectResources[i].extension.uniform.members, size);
      // replacing the data with a pointer which represent the offset
      void *oldPtr = metadata.objectResources[i].extension.uniform.members;
      metadata.objectResources[i].extension.uniform.members =
          reinterpret_cast<SirEngine::graphics::MaterialMetadataStructMember *>(
              (data.data() + counter) - data.data());

      SirEngine::globals::PERSISTENT_ALLOCATOR->free(oldPtr);
      counter += size;
    }
  }

  MaterialMappedData mapped{};
  // we copied the strings and now can copy the array data
  uint32_t size = metadata.objectResourceCount *
                  sizeof(SirEngine::graphics::MaterialResource);
  memcpy(data.data() + counter, metadata.objectResources, size);
  mapped.objectResourceDataOffset = static_cast<uint32_t>(counter);
  counter += size;

  size = metadata.frameResourceCount *
         sizeof(SirEngine::graphics::MaterialResource);
  memcpy(data.data() + counter, metadata.frameResources, size);
  mapped.frameResourceDataOffset = static_cast<uint32_t>(counter);
  counter += size;

  size = metadata.passResourceCount *
         sizeof(SirEngine::graphics::MaterialResource);
  memcpy(data.data() + counter, metadata.passResources, size);
  mapped.passResourceDataOffset = static_cast<uint32_t>(counter);
  counter += size;

  assert(counter == totalSize);
  mapped.objectResourceCount = metadata.objectResourceCount;
  mapped.frameResourceCount = metadata.frameResourceCount;
  mapped.passResourceCount = metadata.passResourceCount;
  mapped.meshBinding = metadata.meshBinding.binding;
  mapped.meshFlags = metadata.meshBinding.flags;

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
        outputPath + graphicsDirectory + fileName + ".metadata";
    compileVK(path, currOutputPath);
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
    // compileAndSavePSO(path, currOutputPath, shaderPath);
    // here we call the compileVK just because for now we are just extracting
    // metadata from the PSO no need to do anything else, once we start
    // pre-compiling and caching psos then we will actually have a dx12 function
    compileVK(path, currOutputPath);
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
    if (target == "DX12") {
      assert(0);
      return compileDx12(assetPath, outputPath, target, shaderPath);
    } else {
      return compileVK(assetPath, outputPath);
    }
  }

  if (target == "DX12") {
    return compileAllDx12(assetPath, outputPath, target, shaderPath);
  }
  return compileAllVK(assetPath, outputPath, target, shaderPath);
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &process);
  return true;
}
