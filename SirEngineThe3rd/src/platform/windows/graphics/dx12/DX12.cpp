#include "SirEnginepch.h"

#include "platform/windows/graphics/dx12/DX12.h"

namespace dx12
{
  ID3D12Device5 *device = nullptr;
  ID3D12Debug *debugController = nullptr;
  IDXGIFactory6 *dxiFactory = nullptr;
}
