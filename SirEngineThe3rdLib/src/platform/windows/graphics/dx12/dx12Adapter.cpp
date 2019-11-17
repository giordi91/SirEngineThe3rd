
#include "platform/windows/graphics/dx12/dx12Adapter.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include <cassert>
#include <d3d12.h>
#include <wrl.h>

namespace SirEngine::dx12 {

bool createWarpDevice(Dx12AdapterResult &adapterResult) {
  // falling back to WARP device
  HRESULT result = DXGI_FACTORY->EnumWarpAdapter(
      IID_PPV_ARGS(&adapterResult.m_physicalDevice));
  if (FAILED(result)) {
    SE_CORE_ERROR("Could not create warp adapter");
    return false;
  }

  result =
      D3D12CreateDevice(adapterResult.m_physicalDevice, D3D_FEATURE_LEVEL_12_1,
                        IID_PPV_ARGS(&adapterResult.m_device));
  if (FAILED(result)) {
    SE_CORE_ERROR("Could not create warp device");
    return false;
  }
  return true;
}

bool createDevice(Dx12AdapterResult &adapterResult) {
  HRESULT result =
      D3D12CreateDevice(adapterResult.m_physicalDevice, D3D_FEATURE_LEVEL_12_1,
                        IID_PPV_ARGS(&adapterResult.m_device));
  if (FAILED(result)) {
    SE_CORE_ERROR("Could not create device with requested features, fallback "
                  "to warp adapter");
    // falling back to WARP device
    return createWarpDevice(adapterResult);
  }
  return true;
}

bool filterBestAdapterByVendor(const AdapterRequestConfig &config,
                               Dx12AdapterResult &adapterResult,
                               IDXGIFactory4 *dxgiFactory) {

  assert(config.m_vendor != ADAPTER_VENDOR::ANY);
  IDXGIAdapter1 *curAdapter;
  size_t adapterMemory = 0;
  IDXGIAdapter3 *dxgiAdapter = nullptr;
  // lets loop the adapters
  for (UINT adapterIdx = 0;
       dxgiFactory->EnumAdapters1(adapterIdx, &curAdapter) == S_OK;
       ++adapterIdx) {

    if (curAdapter == nullptr) {
      break;
    }

    IDXGIAdapter3 *adapter = (IDXGIAdapter3 *)curAdapter;
    DXGI_ADAPTER_DESC desc;
    if (SUCCEEDED(adapter->GetDesc(&desc))) {

      // if not right vendor lets skip it
      uint32_t vendor = VENDOR_ID[static_cast<int>(config.m_vendor)];
      if (vendor != desc.VendorId) {
        continue;
      }

      // if we are here means we have a good adpater
      if (config.m_genericRule == ADAPTER_SELECTION_RULE::FIRST_VALID) {
        adapterResult.m_physicalDevice = adapter;
        createDevice(adapterResult);
        if (adapterResult.m_device == nullptr) {
          exit(EXIT_FAILURE);
        }
        return true;
      }

      // we need to check memory size
      if (desc.DedicatedVideoMemory > adapterMemory) {
        dxgiAdapter = adapter;
        adapterMemory = desc.DedicatedVideoMemory;
      }
    }
  }

  // ok if we got here it can mean few things, that if rule is first valid
  // we did not find any adapter and we should return
  if (config.m_genericRule == ADAPTER_SELECTION_RULE::FIRST_VALID) {
    return false;
  }

  // instead if the rule was not first valid, must be higher memory
  if (adapterMemory > 0 && dxgiAdapter != nullptr) {

    adapterResult.m_physicalDevice = dxgiAdapter;
    bool result = createDevice(adapterResult);
    if (!result) {
      exit(EXIT_FAILURE);
    }
    return true;
  }
  return false;
}

bool getBestAdapter(const AdapterRequestConfig &config,
                    Dx12AdapterResult &adapterResult,
                    IDXGIFactory4 *dxgiFactory) {
  IDXGIAdapter1 *curAdapter;
  size_t adapterMemory = 0;
  IDXGIAdapter3 *dxgiAdapter = nullptr;

  if (config.m_vendor != ADAPTER_VENDOR::ANY) {
    bool result = filterBestAdapterByVendor(config, adapterResult, dxgiFactory);
    if (!result) {
      if (!config.m_vendorTolerant) {

        // no result found for vendor and settings tells us we can't fallback
        SE_CORE_ERROR(
            "Could not find requested vendor {0}, and fail back to any "
            "vendor is not allowed by settings",
            ADAPTER_VENDOR_NAMES[static_cast<int>(config.m_vendor)]);
        exit(EXIT_FAILURE);
      }
      SE_CORE_WARN(
          "Could not find requesed vendor adapter {0}, going for fallback",
          ADAPTER_VENDOR_NAMES[static_cast<int>(config.m_vendor)]);
    }
  }

  // if we are here means we can pick any adapter
  // lets loop the adapters
  for (UINT adapterIdx = 0;
       dxgiFactory->EnumAdapters1(adapterIdx, &curAdapter) == S_OK;
       ++adapterIdx) {

    if (curAdapter == nullptr) {
      break;
    }

    IDXGIAdapter3 *adapter = (IDXGIAdapter3 *)curAdapter;
    DXGI_ADAPTER_DESC desc;
    if (FAILED(adapter->GetDesc(&desc))) {
      SE_CORE_WARN("Failed to fetch description for adapter");
      continue;
    }

    // if software only one is supported and we get out early no matter the
    // rule
    if (desc.VendorId == VENDOR_ID[static_cast<int>(ADAPTER_VENDOR::WARP)]) {
      continue;
    }

    // if we are here means we have a good adpater
    if (config.m_genericRule == ADAPTER_SELECTION_RULE::FIRST_VALID) {
      adapterResult.m_physicalDevice = adapter;
      bool result = createDevice(adapterResult);
      if (!result) {
        exit(EXIT_FAILURE);
      }
      return true;
    }

    if (desc.DedicatedVideoMemory > adapterMemory) {
      dxgiAdapter = adapter;
      adapterMemory = desc.DedicatedVideoMemory;
    }
  }

  if (dxgiAdapter == nullptr) {
    // lets get the warp adapter
    return createWarpDevice(adapterResult);
  }

  // we have a valid adapter lets finish the job
  adapterResult.m_physicalDevice = dxgiAdapter;
  return createDevice(adapterResult);
}

void logPhysicalDevice(IDXGIAdapter3 *physicalDevice) {
  DXGI_ADAPTER_DESC desc;
  HRESULT adapterDescRes = SUCCEEDED(physicalDevice->GetDesc(&desc));
  assert(SUCCEEDED(adapterDescRes));
  char t[128];
  size_t converted = 0;
  wcstombs_s(&converted, t, desc.Description, 128);
  SE_CORE_INFO("Selected GPU: {0}", t);
  SE_CORE_INFO("    vram: {0}GB", desc.DedicatedVideoMemory* 1.0e-9);
  SE_CORE_INFO("    system memory: {0}GB", desc.DedicatedSystemMemory* 1.0e-9);
  SE_CORE_INFO("    shared system memory: {0}GB", desc.SharedSystemMemory* 1.0e-9);
}
} // namespace SirEngine::dx12
