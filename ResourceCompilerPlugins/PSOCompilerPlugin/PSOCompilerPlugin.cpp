#include "PSOCompilerPlugin.h"
#include "PSOProcess.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"

#include "SirEngine/argsUtils.h"
#include "SirEngine/binary/binaryFile.h"
#include "platform/windows/graphics/dx12/dx12Adapter.h"
#include "platform/windows/graphics/dx12/shaderLayout.h"

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

  // exits = filePathExists(outputPath);
  // if (!exits) {
  //  SE_CORE_ERROR("[PSO Compiler] : could not find path/file {0}",
  //  outputPath);
  //}

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
  int csNameSize =
      result.ComputeName == nullptr ? 0 : strlen(result.ComputeName) + 1;

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
  if (result.psoType == SirEngine::dx12::PSOType::RASTER) {
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
    memcpy(bulkDataPtr + ptrPos, result.ComputeName, csNameSize);
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

  request.mapperData = &mapperData;
  request.mapperDataSizeInByte = sizeof(PSOMappedData);
  request.fileType = BinaryFileType::PSO;

  writeBinaryFile(request);

  SE_CORE_INFO("PSO successfully compiled ---> {0}", outFilePath);

  delete SirEngine::dx12::SHADER_LAYOUT_REGISTRY;
  adapterResult.m_device->Release();
  adapterResult.m_physicalDevice->Release();
  DXGI_FACTORY->Release();
  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &process);
  return true;
}
