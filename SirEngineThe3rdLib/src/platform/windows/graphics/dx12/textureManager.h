#pragma once

#include "DXTK12/ResourceUploadBatch.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include <vector>

namespace SirEngine {
namespace dx12 {

struct TextureHandle final {
  uint32_t handle;
};

struct DescriptorPair {
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
};

class TextureManager final {
public:
  TextureManager() : batch(dx12::DEVICE) {}
  ~TextureManager() = default;
  TextureManager(const TextureManager &) = delete;
  TextureManager &operator=(const TextureManager &) = delete;
  void loadTexturesInFolder(const char *path, const char *extension);
  TextureHandle loadTexture(const char *path, bool dynamic = false);

  inline void assertMagicNumber(TextureHandle handle) {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_staticStorage[idx].magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }
  inline uint32_t getIndexFromHandle(TextureHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(TextureHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }

  DescriptorPair getSRV(TextureHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    D3DBuffer buffer;
    buffer.resource = m_staticStorage[index].resource;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->createTexture2DSRV(
        &buffer, m_staticStorage[index].format);
    DescriptorPair pair;
    pair.cpuHandle = buffer.cpuDescriptorHandle;
    pair.gpuHandle = buffer.gpuDescriptorHandle;
    return pair;
  }
  void freeSRV(TextureHandle handle, DescriptorPair pair) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    D3DBuffer buffer;
    buffer.cpuDescriptorHandle = pair.cpuHandle;
    buffer.gpuDescriptorHandle = pair.gpuHandle;
    buffer.descriptorType = DescriptorType::SRV;
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescritpor(buffer);
  }

  inline TextureHandle getHandleFromName(const char *name) {
    auto found = m_nameToHandle.find(name);
    if (found != m_nameToHandle.end()) {
      return found->second;
    }
    return TextureHandle{0};
  }

private:
  struct TextureData final {
    uint32_t magicNumber : 16;
    ID3D12Resource *resource;
    DXGI_FORMAT format;
    bool isGamma : 1;
    bool isHDR : 1;
    uint32_t padding : 14;
  };

private:
  // if we have dynamic storage we are store descriptors upfront
  std::vector<TextureData> m_dynamicStorage[FRAME_BUFFERS_COUNT];
  std::vector<DescriptorPair> m_descriptorStorage;

  std::vector<TextureData> m_staticStorage;
  std::unordered_map<std::string, TextureHandle> m_nameToHandle;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  DirectX::ResourceUploadBatch batch;
}; // namespace dx12

} // namespace dx12
} // namespace SirEngine
