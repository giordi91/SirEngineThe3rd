#include "SirEnginepch.h"

#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/adapter.h"

namespace SirEngine {
namespace dx12 {
ID3D12Device5 *device = nullptr;
ID3D12Debug *debugController = nullptr;
IDXGIFactory6 *dxiFactory = nullptr;
Adapter *adapter = nullptr;

bool initializeGraphics() {

  IDXGIFactory6 *m_dxiFactory = nullptr;
  HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&dxiFactory));
  if (FAILED(result)) {
    return false;
  }

  adapter = new Adapter();
  adapter->setFeture(AdapterFeature::ANY);
  adapter->setVendor(AdapterVendor::ANY);
  adapter->findBestAdapter(dxiFactory);

  result = D3D12CreateDevice(adapter->getAdapter(), D3D_FEATURE_LEVEL_12_1,
                             IID_PPV_ARGS(&device));
  if (FAILED(result)) {
    // falling back to WARP device
    IDXGIAdapter *warpAdapter;
    result = m_dxiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
    if (FAILED(result)) {
      return false;
    }

    result = D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_12_1,
                               IID_PPV_ARGS(&device));
    if (FAILED(result)) {
      return false;
    }
  }

  // Check the maximum feature level, and make sure it's above our minimum
  D3D_FEATURE_LEVEL featureLevelsArray[1];
  featureLevelsArray[0] = D3D_FEATURE_LEVEL_12_1;
  D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {};
  featureLevels.NumFeatureLevels = 1;
  featureLevels.pFeatureLevelsRequested = featureLevelsArray;
  HRESULT r = device->CheckFeatureSupport(
      D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels));
  assert(featureLevels.MaxSupportedFeatureLevel == D3D_FEATURE_LEVEL_12_1);

  if (adapter->getFeature() == AdapterFeature::DXR) {
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 opts5 = {};
    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &opts5,
                                sizeof(opts5));
    if (opts5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
      assert(0);
  }
  return true;
}
} // namespace dx12
} // namespace SirEngine
