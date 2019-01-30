#pragma once
#include <dxgi1_6.h>

// forward
namespace SirEngine {
namespace dx12 {
enum class AdapterVendor { NVIDIA, AMD, INTEL, ANY };

enum class AdapterFeature { DXR = 2, ANY = 4 };

class Adapter {
public:
  Adapter() = default;
  ~Adapter();
  inline void setVendor(AdapterVendor vendor) { m_vendor = vendor; }
  inline void setFeture(AdapterFeature feature) { m_feature = feature; }
  inline AdapterVendor getVendor() const { return m_vendor; }
  inline AdapterFeature getFeature() const { return m_feature; }

  bool findBestAdapter(IDXGIFactory4 *dxgiFactory, bool verbose = false);
  inline IDXGIAdapter3 *getAdapter() const { return m_adapter; }

private:
  AdapterVendor m_vendor = AdapterVendor::NVIDIA;
  AdapterFeature m_feature = AdapterFeature::ANY;
  IDXGIAdapter3 *m_adapter = nullptr;
};
} // namespace dx12
} // namespace SirEngine
