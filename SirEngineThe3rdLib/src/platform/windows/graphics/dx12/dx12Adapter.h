#pragma once
#include "SirEngine/graphics/graphicsDefines.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include <dxgi1_6.h>

// forward
namespace SirEngine::dx12 {

struct Dx12AdapterResult {
  IDXGIAdapter3 *m_physicalDevice;
  D3D12DeviceType *m_device;
};

bool SIR_ENGINE_API getBestAdapter(const AdapterRequestConfig &config,
                    Dx12AdapterResult &adapterResult,
                    IDXGIFactory4 *dxgiFactory);
void SIR_ENGINE_API logPhysicalDevice(IDXGIAdapter3 *physicalDevice);

} // namespace SirEngine::dx12
