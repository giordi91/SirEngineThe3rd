#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

namespace SirEngine {
namespace dx12 {
class Adapter;

extern ID3D12Device5 *device;
extern ID3D12Debug *debugController;
extern IDXGIFactory6 *dxiFactory;
extern Adapter *adapter;

bool initializeGraphics();

// CommandQueue m_commandQueue;
} // namespace dx12
} // namespace SirEngine
