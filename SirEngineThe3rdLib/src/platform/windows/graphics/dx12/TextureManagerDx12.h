#pragma once

#include "DXTK12/ResourceUploadBatch.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/SparseMemoryPool.h"
#include "SirEngine/textureManager.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"

namespace SirEngine {
namespace dx12 {

class TextureManagerDx12 final : public TextureManager {
  struct TextureData final {
    uint32_t magicNumber;
    ID3D12Resource *resource;
    D3D12_RESOURCE_STATES state;
    DXGI_FORMAT format;
    TextureFlags flags;
    DescriptorPair srv;
    DescriptorPair uav;
  };

public:
  TextureManagerDx12() : batch(dx12::DEVICE), m_texturePool(RESERVE_SIZE) {
    m_nameToHandle.reserve(RESERVE_SIZE);
  }
  ~TextureManagerDx12();
  TextureManagerDx12(const TextureManagerDx12 &) = delete;
  TextureManagerDx12 &operator=(const TextureManagerDx12 &) = delete;
  virtual TextureHandle loadTexture(const char *path) override;
  virtual void free(const TextureHandle handle) override;
  virtual TextureHandle allocateRenderTexture(uint32_t width, uint32_t height,
                                              RenderTargetFormat format,
                                              const char *name) override;
  virtual void bindRenderTarget(TextureHandle handle,
                                TextureHandle depth) override;
  virtual void copyTexture(TextureHandle source,
                           TextureHandle destination) override;
  virtual void bindBackBuffer(bool bindBackBufferDepth) override;
  virtual void clearDepth(const TextureHandle depth) override;
  virtual void clearRT(const TextureHandle handle, const float color[4])override;

  // dx12 methods
  TextureHandle initializeFromResourceDx12(ID3D12Resource *resource,
                                           const char *name,
                                           D3D12_RESOURCE_STATES state);
  TextureHandle
  createDepthTexture(const char *name, uint32_t width, uint32_t height,
                     D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);

  // handles facilities
  DescriptorPair getSRVDx12(const TextureHandle handle) {

    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
	assert(m_texturePool.getConstRef(index).srv.cpuHandle.ptr != 0);
	assert(m_texturePool.getConstRef(index).srv.type == DescriptorType::SRV);
    return m_texturePool.getConstRef(index).srv;
  }
  // A manual format is passed to the depth becauase we normally use a typess
  // type so we cannot rely on the format used during allocation.
  DescriptorPair
  getDSVDx12(const TextureHandle handle,
             const DXGI_FORMAT format = DXGI_FORMAT_D24_UNORM_S8_UINT) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_texturePool[index].srv;
  }
  void freeSRVDx12(const TextureHandle handle,
                   const DescriptorPair pair) const {
    assertMagicNumber(handle);
    assert(pair.type == DescriptorType::SRV);
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(pair);
  }
  inline DescriptorPair getRTVDx12(const TextureHandle handle) const {

    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_texturePool.getConstRef(index).srv;
  }
  void freeRTVDx12(const TextureHandle handle,
                   const DescriptorPair pair) const {
    assertMagicNumber(handle);
    dx12::GLOBAL_RTV_HEAP->freeDescriptor(pair);
    // TODO should I invalidate the descriptors here??
  }
  void freeDSVDx12(const TextureHandle handle,
                   const DescriptorPair pair) const {
    assertMagicNumber(handle);
    dx12::GLOBAL_DSV_HEAP->freeDescriptor(pair);
    assert(pair.type == DescriptorType::DSV);
  }

  // barriers
  inline int
  transitionTexture2DifNeeded(const TextureHandle handle,
                              const D3D12_RESOURCE_STATES wantedState,
                              D3D12_RESOURCE_BARRIER *barriers, int counter) {

    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    TextureData &data = m_texturePool[index];

    auto state = data.state;
    if (state != wantedState) {
      barriers[counter] = CD3DX12_RESOURCE_BARRIER::Transition(
          data.resource, state, wantedState);
      data.state = wantedState;
      ++counter;
    }
    return counter;
  }

private:
  inline void assertMagicNumber(const TextureHandle handle) const {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_texturePool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }

private:
  std::unordered_map<std::string, TextureHandle> m_nameToHandle;
  DirectX::ResourceUploadBatch batch;
  SparseMemoryPool<TextureData> m_texturePool;
};

} // namespace dx12
} // namespace SirEngine
