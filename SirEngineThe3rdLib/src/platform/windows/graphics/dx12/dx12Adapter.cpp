
#include "platform/windows/graphics/dx12/dx12Adapter.h"
#include <cassert>
#include <d3d12.h>
#include <iostream>
#include <wrl.h>

namespace SirEngine::dx12 {
Dx12Adapter::~Dx12Adapter() {
  if (m_adapter != nullptr) {
    m_adapter->Release();
  }
}
bool Dx12Adapter::findBestAdapter(IDXGIFactory4 *dxgiFactory, bool verbose) {

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

    // inspecting the description
    DXGI_ADAPTER_DESC desc;
    if (SUCCEEDED(adapter->GetDesc(&desc))) {
      bool isDXR = false;
      bool requiresDXR = m_feature == AdapterFeature::DXR;
      // for now only checking DXR assuming Nvidia
      if (requiresDXR) {
        isDXR = (wcsstr(desc.Description, L"RTX") != 0);
      }

      std::wcout << desc.Description << std::endl;
      // checking for Microsoft software adapter, we want to skip it
      bool isSoftwareVendor = desc.VendorId == 0x1414;
      bool isSoftwareId = desc.DeviceId == 0x8c;
      bool isSoftware = isSoftwareVendor & isSoftwareId;
      if (isSoftware && (m_vendor == ADAPTER_VENDOR::WARP)) {
        m_adapter = adapter;
        DXGI_ADAPTER_DESC1 desc1;
        adapter->GetDesc1(&desc1);
        assert((desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE));
        return true;
      }

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
} // namespace SirEngine::dx12
