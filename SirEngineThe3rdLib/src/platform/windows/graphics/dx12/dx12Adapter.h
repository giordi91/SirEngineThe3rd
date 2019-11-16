#pragma once
#include <dxgi1_6.h>
#include "SirEngine/graphics/graphicsDefines.h"

// forward
namespace SirEngine {
namespace dx12 {

enum class AdapterFeature { DXR = 2, ANY = 4 };

class Dx12Adapter {
public:
  Dx12Adapter() = default;
  ~Dx12Adapter();
  inline void setVendor(ADAPTER_VENDOR vendor) { m_vendor = vendor; }
  inline void setFeture(AdapterFeature feature) { m_feature = feature; }
  inline ADAPTER_VENDOR getVendor() const { return m_vendor; }
  inline AdapterFeature getFeature() const { return m_feature; }

  bool findBestAdapter(IDXGIFactory4 *dxgiFactory, bool verbose = false);
  inline IDXGIAdapter3 *getAdapter() const { return m_adapter; }

private:
  ADAPTER_VENDOR m_vendor = ADAPTER_VENDOR::NVIDIA;
  AdapterFeature m_feature = AdapterFeature::ANY;
  IDXGIAdapter3 *m_adapter = nullptr;
};
} // namespace dx12
} // namespace SirEngine
