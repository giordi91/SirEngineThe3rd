#pragma once

#include "DXTK12/ResourceUploadBatch.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/SparseMemoryPool.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <vector>

namespace SirEngine {
namespace dx12 {

struct TextureHandle final {
  uint32_t handle;
};

enum DebugTextureFlags { DEPTH = 1 };

class TextureManager final {
private:
  struct TextureData final {
    uint32_t magicNumber;
    ID3D12Resource *resource;
    D3D12_RESOURCE_STATES state;
    DXGI_FORMAT format;
    DebugTextureFlags flags;
  };

public:
  TextureManager() : batch(dx12::DEVICE), m_texturePool(RESERVE_SIZE) {
    m_nameToHandle.reserve(RESERVE_SIZE);
  }
  ~TextureManager() = default;
  TextureManager(const TextureManager &) = delete;
  TextureManager &operator=(const TextureManager &) = delete;
  TextureHandle loadTexture(const char *path, bool dynamic = false);
  TextureHandle initializeFromResource(ID3D12Resource *resource,
                                       const char *name,
                                       D3D12_RESOURCE_STATES state);
  TextureHandle
  createDepthTexture(const char *name, uint32_t width, uint32_t height,
                     D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);

  inline void assertMagicNumber(TextureHandle handle) const {
#ifdef SE_DEBUG
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_texturePool.getConstRef(idx).magicNumber == magic &&
           "invalid magic handle for constant buffer");
#endif
  }
  inline uint32_t getIndexFromHandle(TextureHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(TextureHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }

  // handles facilities
  DescriptorPair getSRV(TextureHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    DescriptorPair pair;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(
        pair, m_texturePool[index].resource, m_texturePool[index].format);
    return pair;
  }
  // A manual format is passed to the depth becauase we normally use a typess
  // type so we cannot rely on the format used during allocation.
  DescriptorPair getDSV(TextureHandle handle,
                        DXGI_FORMAT format = DXGI_FORMAT_D24_UNORM_S8_UINT) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    DescriptorPair pair;
    dx12::createDSV(dx12::GLOBAL_DSV_HEAP, m_texturePool[index].resource,
                    pair, format);
    return pair;
  }
  void freeSRV(TextureHandle handle, DescriptorPair pair) const {
    assertMagicNumber(handle);
    assert(pair.type == DescriptorType::SRV);
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(pair);
  }
  inline DescriptorPair getRTV(TextureHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    DescriptorPair pair;
    dx12::createRTVSRV(dx12::GLOBAL_RTV_HEAP, m_texturePool.getConstRef(index).resource,
                       pair);
    return pair;
  }
  void freeRTV(const TextureHandle handle, const DescriptorPair pair) const {
    assertMagicNumber(handle);
    dx12::GLOBAL_RTV_HEAP->freeDescriptor(pair);
    // TODO should I invalidate the descriptors here??
  }
  void freeDSV(const TextureHandle handle, const DescriptorPair pair) const {
    assertMagicNumber(handle);
    dx12::GLOBAL_DSV_HEAP->freeDescriptor(pair);
    assert(pair.type == DescriptorType::DSV);
  }
  inline TextureHandle getHandleFromName(const char *name) {
    auto found = m_nameToHandle.find(name);
    if (found != m_nameToHandle.end()) {
      return found->second;
    }
    return TextureHandle{0};
  }

  // barriers
  inline int transitionTexture2DifNeeded(TextureHandle handle,
                                         D3D12_RESOURCE_STATES wantedState,
                                         D3D12_RESOURCE_BARRIER *barriers,
                                         int counter) {

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

  void free(const TextureHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    TextureData &data = m_texturePool[index];
    // releasing the texture;
    data.resource->Release();
    // invalidating magic number
    data.magicNumber = 0;

    // adding the index to the free list
	m_texturePool.free(index);
    //m_freeSlots[m_freeSlotIndex++] = index;
  }

  TextureData &TextureManager::getFreeTextureData(uint32_t &index);

private:
  // if we have dynamic storage we are store descriptors upfront
  // std::vector<TextureData> m_dynamicStorage[FRAME_BUFFERS_COUNT];
  // std::vector<DescriptorPair> m_descriptorStorage;

  // std::vector<TextureData> m_staticStorage;
  std::unordered_map<std::string, TextureHandle> m_nameToHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  DirectX::ResourceUploadBatch batch;
  SparseMemoryPool<TextureData> m_texturePool;
}; // namespace dx12

} // namespace dx12
} // namespace SirEngine
