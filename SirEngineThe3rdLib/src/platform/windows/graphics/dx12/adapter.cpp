
#include "platform/windows/graphics/dx12/adapter.h"
#include <d3d12.h>
#include <iostream>
#include <wrl.h>

namespace SirEngine {
namespace dx12 {
Adapter::~Adapter() {
  if (m_adapter != nullptr) {
    m_adapter->Release();
  }
}
bool Adapter::findBestAdapter(IDXGIFactory4 *dxgiFactory, bool verbose) {

  IDXGIAdapter1 *curAdapter;
  size_t adapterMemory = 0;
  IDXGIAdapter3 *dxgiAdapter = nullptr;
  // lets loop the adapters
  for (UINT adapterIdx = 0;
       dxgiFactory->EnumAdapters1(adapterIdx, &curAdapter) == S_OK;
       ++adapterIdx) {

    IDXGIAdapter3 *adapter = (IDXGIAdapter3 *)curAdapter;
    if (adapter == nullptr) {
      break;
    }

    // inspecting the description
    DXGI_ADAPTER_DESC desc;
    if (SUCCEEDED(adapter->GetDesc(&desc))) {
      bool isDXR = false;
      bool requiresDXR = m_feature == AdapterFeature::DXR;
      // for now only checking DXR assuming Nvidia
      if (requiresDXR) {
        isDXR = (wcsstr(desc.Description, L"RTX") != 0);
      }

      // checking for Microsoft software adapter, we want to skip it
      bool isSoftwareVendor = desc.VendorId == 0x1414;
      bool isSoftwareId = desc.DeviceId == 0x8c;
      bool isSoftware = isSoftwareVendor & isSoftwareId;
      if (!((isSoftware) & (isDXR & requiresDXR))) {
        // then we just prioritize memory size, in the future we
        // might want to use also other metrics
        if (desc.DedicatedVideoMemory > adapterMemory) {
          dxgiAdapter = adapter;
          adapterMemory = desc.DedicatedVideoMemory;
        }
      }
    }
  }
  m_adapter = dxgiAdapter;

  if (verbose) {
    DXGI_ADAPTER_DESC desc;
    m_adapter->GetDesc(&desc);
    std::wcout << "[VIDEO] current adapter " << desc.Description << std::endl;
  }
  return dxgiAdapter != nullptr;
}
} // namespace dx12
} // namespace SirEngine
