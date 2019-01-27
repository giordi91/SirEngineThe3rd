#pragma once
//#include "Illuminati/system/debug.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include <dxgiformat.h>

struct ID3D12Resource;
namespace SirEngine {

namespace dx12 {

class Texture2D {

public:
  Texture2D()=default;
  ~Texture2D();
  bool initializeEmpty(int width, int height, DXGI_FORMAT format);

  bool initializeEmptyRT(int width, int height, DXGI_FORMAT format);
  bool initializeFromResource(ID3D12Resource *resource,
                              DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

  bool initializeRTFromResource(ID3D12Resource *resource);
  void clear();

  inline ID3D12Resource *getResource() { return m_texture.resource; };
  inline D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptor() {
    return m_texture.gpuDescriptorHandle;
  };
  inline D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptorSRV() {
    return m_textureSRV.gpuDescriptorHandle;
  };

  inline D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptorSRV() {
    return m_textureSRV.cpuDescriptorHandle;
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
