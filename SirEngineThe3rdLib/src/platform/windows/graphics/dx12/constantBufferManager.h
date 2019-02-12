#pragma once
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include <vector>
#include "SirEngine/handle.h"

namespace SirEngine {
namespace dx12 {


class ConstantBufferManager final {
public:
  ConstantBufferManager();
  ~ConstantBufferManager() = default;
  ConstantBufferManager(const ConstantBufferManager &) = delete;
  ConstantBufferManager &operator=(const ConstantBufferManager &) = delete;

  ConstantBufferHandle allocateDynamic(uint32_t sizeInBytes);

  inline void assertMagicNumber(ConstantBufferHandle handle) {
    uint32_t magic = getMagicFromHandel(handle);
    uint32_t idx = getIndexFromHandel(handle);
    assert(m_dynamicStorage[globals::CURRENT_FRAME][idx].magicNumber == magic &&
           "invalid magic handle for constant buffer");
  }

  inline DescriptorPair
  getConstantBufferDescriptor(ConstantBufferHandle handle) {

    // making sure the resource has not been de-allocated
    assertMagicNumber(handle);

    uint32_t index = getIndexFromHandel(handle);
    uint32_t dIndex =
        m_dynamicStorage[globals::CURRENT_FRAME][index].descriptorIndex;
    return m_descriptorStorage[dIndex];
  }

  inline void updateConstantBuffer(ConstantBufferHandle handle,
                                   void *dataToUpload) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandel(handle);
    const ConstantBufferData &data =
        m_dynamicStorage[globals::CURRENT_FRAME][index];

    assert(data.mappedData != nullptr);
    memcpy(data.mappedData, dataToUpload, data.size);
  }

private:
  struct ConstantBufferData final {
    // we are using one byte for the mapped flag and 31 bytes for the
    // the actual data size, we can't have buffers that big anyway
    bool mapped : 1;
    uint32_t size : 31;
    uint32_t descriptorIndex : 16;
    uint32_t magicNumber : 16;
    uchar *mappedData = nullptr;
    ID3D12Resource *resource = nullptr;
  };

private:
  inline uint32_t getIndexFromHandel(ConstantBufferHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandel(ConstantBufferHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }
  inline void mapConstantBuffer(ConstantBufferData &data) const {
    if (!data.mapped) {
      data.mapped = true;
      data.resource->Map(0, nullptr,
                         reinterpret_cast<void **>(&data.mappedData));
    } else {
      SE_CORE_WARN("Tried to map an already mapped buffer");
    }
  };

private:
  std::vector<ConstantBufferData> m_dynamicStorage[FRAME_BUFFERS_COUNT];
  std::vector<DescriptorPair> m_descriptorStorage;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
  static const uint32_t RESERVE_SIZE = 200;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
};

} // namespace dx12
} // namespace SirEngine
