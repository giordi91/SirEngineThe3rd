#pragma once

#include "DXTK12/ResourceUploadBatch.h"
#include "SirEngine/core.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/textureManager.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"

namespace SirEngine {
namespace dx12 {

class  Dx12TextureManager final : public TextureManager {
  struct TextureData final {
    uint32_t magicNumber;
    ID3D12Resource *resource;
    D3D12_RESOURCE_STATES state;
    DXGI_FORMAT format;
    uint32_t flags;  // combination of texture allocation flags
    DescriptorPair srv;
    DescriptorPair rtsrv;
    DescriptorPair uav;
    DescriptorPair dsvStencil;
    uint32_t width;
    uint32_t height;
  };

 public:
  Dx12TextureManager()
      : TextureManager(), batch(dx12::DEVICE), m_texturePool(RESERVE_SIZE) {}
  virtual ~Dx12TextureManager();
  Dx12TextureManager(const Dx12TextureManager &) = delete;
  Dx12TextureManager &operator=(const Dx12TextureManager &) = delete;
  virtual TextureHandle loadTexture(const char *path,
                                    bool cubeMap = false) override;
  void free(const TextureHandle handle) override;
  TextureHandle allocateTexture(uint32_t width, uint32_t height,
                                RenderTargetFormat format, const char *name,
                                uint32_t allocFlags,
                                RESOURCE_STATE finalState) override;

  void clearDepth(const TextureHandle depth, const float depthValue,
                  const float stencilValue) const;
  void clearRT(const TextureHandle handle, const float color[4]) const;

  void initialize() override;
  void cleanup() override;
  TextureHandle getWhiteTexture() const override { return m_whiteTexture; }

  void transitionTexture(CommandBufferHandle commandBuffer, TextureHandle texHandle, RESOURCE_STATE currState,
	  RESOURCE_STATE newState) override;


	
  // dx12 methods
  TextureHandle initializeFromResourceDx12(ID3D12Resource *resource,
                                           const char *name,
                                           D3D12_RESOURCE_STATES state);
  void bindBackBuffer() const;
  ;

  // handles facilities
  DescriptorPair getSRVDx12(const TextureHandle handle) {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const TextureData &data = m_texturePool.getConstRef(index);
    assert(data.srv.cpuHandle.ptr != 0);
    assert(data.srv.type == DescriptorType::SRV);
    return m_texturePool.getConstRef(index).srv;
  }
  DescriptorPair getSrvStencilDx12(const TextureHandle handle);
  DescriptorPair getUAVDx12(const TextureHandle handle) {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const TextureData &data = m_texturePool.getConstRef(index);
    assert(data.uav.cpuHandle.ptr != 0);
    assert(data.uav.type == DescriptorType::UAV);
    return m_texturePool.getConstRef(index).uav;
  }
  ID3D12Resource *getRawTexture(const TextureHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    const TextureData &data = m_texturePool.getConstRef(index);
    return data.resource;
  }

  // A manual format is passed to the depth becauase we normally use a typess
  // type so we cannot rely on the format used during allocation.
  DescriptorPair getDSVDx12(const TextureHandle handle) {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
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
    const uint32_t index = getIndexFromHandle(handle);
    return m_texturePool.getConstRef(index).rtsrv;
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
  inline int transitionTexture2DifNeeded(
      const TextureHandle handle, const D3D12_RESOURCE_STATES wantedState,
      D3D12_RESOURCE_BARRIER *barriers, int counter) {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    TextureData &data = m_texturePool[index];

    if (data.state != wantedState) {
      barriers[counter] = CD3DX12_RESOURCE_BARRIER::Transition(
          data.resource, data.state, wantedState);
      data.state = wantedState;
      ++counter;
    }
    return counter;
  }
  void createSRV(const TextureHandle handle, DescriptorPair &pair,
                 const bool isCubeMap) {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    // const uint32_t index = getIndexFromHandle(m_whiteTexture);
    TextureData &data = m_texturePool[index];
    if (!isCubeMap) {
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(pair, data.resource,
                                                        data.format, 0, true);
    } else {
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTextureCubeSRV(pair, data.resource,
                                                          data.format, true);
    }
  }

  D3D12_RESOURCE_STATES toDx12ResourceState(const RESOURCE_STATE state);

 private:
  inline void assertMagicNumber(const TextureHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(m_texturePool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }

 private:
  DirectX::ResourceUploadBatch batch;
  SparseMemoryPool<TextureData> m_texturePool;

  // default texture
  TextureHandle m_whiteTexture;
};

}  // namespace dx12
}  // namespace SirEngine
