#pragma once

#include "DXTK12/ResourceUploadBatch.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <vector>

namespace SirEngine {
namespace dx12 {

struct TextureHandle final {
  uint32_t handle;
};

class TextureManager final {
private:
  struct TextureData final {
    uint32_t magicNumber;
    ID3D12Resource *resource;
    D3D12_RESOURCE_STATES state;
    DXGI_FORMAT format;
  };

public:
  TextureManager() : batch(dx12::DEVICE) {
    m_staticStorage.reserve(RESERVE_SIZE);
    m_nameToHandle.reserve(RESERVE_SIZE);
    m_freeSlots.resize(RESERVE_SIZE);
  }
  ~TextureManager() = default;
  TextureManager(const TextureManager &) = delete;
  TextureManager &operator=(const TextureManager &) = delete;
  TextureHandle loadTexture(const char *path, bool dynamic = false);
  TextureHandle initializeFromResource(ID3D12Resource *resource,
                                       const char *name,
                                       D3D12_RESOURCE_STATES state);

  inline void assertMagicNumber(TextureHandle handle) const {
#ifdef _DEBUG
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_staticStorage[idx].magicNumber == magic &&
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
        pair, m_staticStorage[index].resource, m_staticStorage[index].format);
    return pair;
  }
  void freeSRV(TextureHandle handle, DescriptorPair pair) {
    assertMagicNumber(handle);
    D3DBuffer buffer;
    buffer.cpuDescriptorHandle = pair.cpuHandle;
    buffer.gpuDescriptorHandle = pair.gpuHandle;
    buffer.descriptorType = DescriptorType::SRV;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescritpor(buffer);
  }
  inline DescriptorPair getRTV(TextureHandle handle) const {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    // TODO get rid of 3d buffer and start working on
    // DescriptorPair only?
    D3DBuffer buffer;
    buffer.resource = m_staticStorage[index].resource;
    dx12::createRTVSRV(dx12::GLOBAL_RTV_HEAP, &buffer);
    DescriptorPair pair;
    pair.cpuHandle = buffer.cpuDescriptorHandle;
    pair.gpuHandle = buffer.gpuDescriptorHandle;
    return pair;
  }
  void freeRTV(const TextureHandle handle, const DescriptorPair pair) const {
    assertMagicNumber(handle);
    dx12::GLOBAL_RTV_HEAP->freeDescriptor(pair);
    // TODO should I invalidate the descriptors here??
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
    TextureData &data = m_staticStorage[index];

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
    TextureData &data = m_staticStorage[index];
    // releasing the texture;
    data.resource->Release();
    // invalidating magic number
    data.magicNumber = 0;
    // adding the index to the free list
    m_freeSlots[m_freeSlotIndex++] = index;
  }

  TextureData &TextureManager::getFreeTextureData(uint32_t &index);

private:
  // if we have dynamic storage we are store descriptors upfront
  // std::vector<TextureData> m_dynamicStorage[FRAME_BUFFERS_COUNT];
  // std::vector<DescriptorPair> m_descriptorStorage;

  std::vector<TextureData> m_staticStorage;
  std::unordered_map<std::string, TextureHandle> m_nameToHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  DirectX::ResourceUploadBatch batch;
  std::vector<uint32_t> m_freeSlots;
  uint32_t m_freeSlotIndex = 0;
}; // namespace dx12

} // namespace dx12
} // namespace SirEngine
