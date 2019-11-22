#include "PSOProcess.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "nlohmann/json.hpp"
#include "platform/windows/graphics/dx12/PSOCompile.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/dx12Adapter.h"
#include "platform/windows/graphics/dx12/shaderLayout.h"
#include <cassert>
#include <string>
#include <unordered_map>

SirEngine::dx12::PSOCompileResult processPSO(const char *path) {

  IDXGIFactory6 *DXGI_FACTORY = nullptr;
  const HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&DXGI_FACTORY));
  if (FAILED(result)) {
    return SirEngine::dx12::PSOCompileResult{nullptr,
                                             SirEngine::dx12::PSOType::INVALID};
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

  // ADAPTER = adapterResult.m_physicalDevice;
  SirEngine::dx12::DEVICE = adapterResult.m_device;
  SirEngine::dx12::SHADER_LAYOUT_REGISTRY =
      new SirEngine::dx12::ShadersLayoutRegistry();

  SirEngine::dx12::PSOCompileResult buildResult =
      SirEngine::dx12::loadPSOFile(path);

  delete SirEngine::dx12::SHADER_LAYOUT_REGISTRY;
  adapterResult.m_device->Release();
  adapterResult.m_physicalDevice->Release();
  DXGI_FACTORY->Release();

  return buildResult;
}
