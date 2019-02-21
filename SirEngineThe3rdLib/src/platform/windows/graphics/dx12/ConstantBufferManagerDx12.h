#pragma once
#include "SirEngine/constantBufferManager.h"
#include "SirEngine/globals.h"
#include "SirEngine/handle.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include <vector>

namespace SirEngine {
namespace dx12 {

class ConstantBufferManagerDx12 final : public ConstantBufferManager {
public:
  ConstantBufferManagerDx12();
  virtual ~ConstantBufferManagerDx12() = default;
  ConstantBufferManagerDx12(const ConstantBufferManagerDx12 &) = delete;
  ConstantBufferManagerDx12 &
  operator=(const ConstantBufferManagerDx12 &) = delete;
  virtual ConstantBufferHandle allocateDynamic(uint32_t sizeInBytes, void* data = nullptr) override;

  inline DescriptorPair
  getConstantBufferDx12Handle(ConstantBufferHandle handle) {

    // making sure the resource has not been de-allocated
    assertMagicNumber(handle);

    uint32_t index = getIndexFromHandle(handle);
    uint32_t dIndex =
        m_dynamicStorage[globals::CURRENT_FRAME][index].descriptorIndex;
    return m_descriptorStorage[dIndex];
  }

  inline D3D12_GPU_VIRTUAL_ADDRESS
  getVirtualAddress(ConstantBufferHandle handle) {
    assertMagicNumber(handle);
    uint32_t index = getIndexFromHandle(handle);
    return m_dynamicStorage[globals::CURRENT_FRAME][index]
        .resource->GetGPUVirtualAddress();
  }

  virtual void updateConstantBuffer(const ConstantBufferHandle handle,
                                    void *dataToUpload) override;

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
  inline void assertMagicNumber(ConstantBufferHandle handle) {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(m_dynamicStorage[globals::CURRENT_FRAME][idx].magicNumber == magic &&
           "invalid magic handle for constant buffer");
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
