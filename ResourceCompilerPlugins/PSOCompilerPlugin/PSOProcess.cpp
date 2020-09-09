#include "PSOProcess.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/PSOCompile.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/dx12Adapter.h"
#include <cassert>

SirEngine::dx12::PSOCompileResult processPSO(const char *path, const char* shaderPath) {

  // init graphics
#if defined(DEBUG) || defined(_DEBUG)
  {
    const HRESULT controllerResult = D3D12GetDebugInterface(
        IID_PPV_ARGS(&SirEngine::dx12::DEBUG_CONTROLLER));
    if (FAILED(controllerResult)) {
      return SirEngine::dx12::PSOCompileResult{
          nullptr, nullptr, nullptr, SirEngine::PSO_TYPE::INVALID};
    }
    SirEngine::dx12::DEBUG_CONTROLLER->EnableDebugLayer();
  }
#endif

  // init graphics
  IDXGIFactory6 *DXGI_FACTORY = nullptr;
  const HRESULT dxgiResult = CreateDXGIFactory1(IID_PPV_ARGS(&DXGI_FACTORY));
  if (FAILED(dxgiResult)) {
    assert(0);
  }

  SirEngine::AdapterRequestConfig adapterConfig{};
  adapterConfig.m_vendor = SirEngine::ADAPTER_VENDOR::ANY;
  adapterConfig.m_vendorTolerant = true;
  adapterConfig.m_genericRule =
      SirEngine::ADAPTER_SELECTION_RULE::LARGEST_FRAME_BUFFER;

  SirEngine::dx12::Dx12AdapterResult adapterResult{};
  const bool foundAdapter =
      getBestAdapter(adapterConfig, adapterResult, DXGI_FACTORY);
  assert(foundAdapter && "could not find adapter matching features");

  SirEngine::dx12::ADAPTER = adapterResult.m_physicalDevice;
  SirEngine::dx12::DEVICE = adapterResult.m_device;

  // Check the maximum feature level, and make sure it's above our minimum
  D3D_FEATURE_LEVEL featureLevelsArray[1];
  featureLevelsArray[0] = D3D_FEATURE_LEVEL_12_1;
  D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {};
  featureLevels.NumFeatureLevels = 1;
  featureLevels.pFeatureLevelsRequested = featureLevelsArray;
  HRESULT r = SirEngine::dx12::DEVICE->CheckFeatureSupport(
      D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels));
  assert(featureLevels.MaxSupportedFeatureLevel == D3D_FEATURE_LEVEL_12_1);
  assert(SUCCEEDED(r));

  // COMPILING
  SirEngine::dx12::PSOCompileResult buildResult =
      SirEngine::dx12::compileRawPSO(path, shaderPath);
  //cleanup 
  adapterResult.m_device->Release();
  adapterResult.m_physicalDevice->Release();
  DXGI_FACTORY->Release();

  return buildResult;
}
