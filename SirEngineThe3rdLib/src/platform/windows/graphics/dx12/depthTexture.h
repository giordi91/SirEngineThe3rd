#pragma once

#include "platform/windows/graphics/dx12/DX12.h"
#include <dxgiformat.h>

struct ID3D12Resource;
namespace SirEngine {
namespace dx12 {

class DepthTexture {

public:
  DepthTexture() = default;
  ~DepthTexture();
  bool initialize( int width, int height);

  void clear();

  inline ID3D12Resource *getResource() { return m_texture.resource; };
  inline D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptor() {
    return m_texture.gpuDescriptorHandle;
  };
  inline D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptorSRV() {
    return m_textureSRV.gpuDescriptorHandle;
  };
  inline D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptor() {
    return m_texture.cpuDescriptorHandle;
  };
  inline D3D12_RESOURCE_STATES getState() { return m_currentState; }
  inline void setState(D3D12_RESOURCE_STATES newState) {
    m_currentState = newState;
  }

private:
  D3DBuffer m_texture;
  D3DBuffer m_textureSRV;
  D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
};

} // namespace dx12
} // namespace SirEngine
