#include "textureCompilerPlugin.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "cxxopts/cxxopts.hpp"

#include "SirEngine/headlessClient.h"
#include "processTexture.h"
#include "resourceCompilerLib/argsUtils.h"

// engine includes
#include "SirEngine/textureManager.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/PSOManager.h"
#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "platform/windows/graphics/dx12/TextureManagerDx12.h"

// debug capture
#include <DXProgrammableCapture.h>

const std::string PLUGIN_NAME = "textureCompilerPlugin";
const unsigned int VERSION_MAJOR = 0;
const unsigned int VERSION_MINOR = 1;
const unsigned int VERSION_PATCH = 0;

void processArgs(const std::string args, std::string &format, bool &isGamma) {
  // lets get arguments like they were from commandline
  auto v = splitArgs(args);
  // lets build the options
  cxxopts::Options options("Texture compiler",
                           "Converts a texture in DDS format");
  options.add_options()("f,format", "output format for the texture",
                        cxxopts::value<std::string>())(
      "g,gamma", "whether the texture is to considered gamma corrected or not",
      cxxopts::value<std::string>()->implicit_value("1"));
  char **argv = v.argv.get();
  auto result = options.parse(v.argc, argv);

  if (result.count("format")) {
    format = result["format"].as<std::string>();
  }
  isGamma = false;
  if (result.count("gamma")) {
    isGamma = true;
  }
}

bool processTexture(const std::string &assetPath, const std::string &outputPath,
                    const std::string &args) {

  // test
  SirEngine::HeadlessClient client;
  SirEngine::dx12::RootSignatureManager *rootM =
      client.getRootSignatureManager();
  SirEngine::dx12::PSOManager *psoM = client.getPSOManager();
  SirEngine::dx12::TextureManagerDx12 *textureManager = client.getTextureManager();

  ID3D12RootSignature *rs = rootM->getRootSignatureFromName("textureMIPS_RS");
  ID3D12PipelineState *pso = psoM->getComputePSOByName("textureMIPS_PSO");

  auto textureHandle = textureManager->allocateRenderTexture(
      1024, 1024, SirEngine::RenderTargetFormat::RGBA32, "dummy",true);

  Microsoft::WRL::ComPtr<IDXGraphicsAnalysis> ga;
  HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&ga));
  assert(SUCCEEDED(hr));

  ga->BeginCapture();
  client.beginWork();
  SirEngine::dx12::FrameResource* frameRes = client.getFrameResource();
  auto commandList = frameRes->fc.commandList;
  commandList->SetPipelineState(pso);
  commandList->SetComputeRootSignature(rs);

  D3D12_RESOURCE_BARRIER barriers[5];
  int counter = 0;
  counter = textureManager->transitionTexture2DifNeeded(
      textureHandle, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barriers,
      counter);

  commandList->SetComputeRootDescriptorTable(2,
	textureManager->getUAVDx12(textureHandle).gpuHandle);
  commandList->Dispatch(128,128,1);

  client.endWork();
  client.flushAllOperation();
  ga->EndCapture();

  // processing plug-ins args
  std::string format;
  bool isGamma;
  processArgs(args, format, isGamma);

  // checking IO files exits
  bool exists = fileExists(assetPath);
  if (!exists) {
    SE_CORE_ERROR("[Texture Compiler] : could not find path/file {0}",
                  assetPath);
  }

  exists = filePathExists(outputPath);
  if (!exists) {
    SE_CORE_ERROR("[Texture Compiler] : could not find path/file {0}",
                  outputPath);
  }
  bool res = processTextureFile(assetPath.c_str(), outputPath.c_str(), format,
                                isGamma);

  if (res) {
    SE_CORE_INFO("Texture successfully compiled ---> {0}", outputPath);
  }
  return true;
}

bool pluginRegisterFunction(PluginRegistry *registry) {
  registry->registerFunction(PLUGIN_NAME, &processTexture);
  return true;
}
