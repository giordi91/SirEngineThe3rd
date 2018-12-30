#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include "platform/windows/graphics/dx12/adapter.h"

namespace dx12
{
  //DescriptorHeap m_cbvSrvUavHeap;
  //DescriptorHeap m_rtvHeap;
  //DescriptorHeap m_dsvHeap;
  //Adapter* m_adapter;
  extern ID3D12Device5 *device;
  extern ID3D12Debug *debugController;
  extern IDXGIFactory6 *dxiFactory;

  //CommandQueue m_commandQueue;
}


