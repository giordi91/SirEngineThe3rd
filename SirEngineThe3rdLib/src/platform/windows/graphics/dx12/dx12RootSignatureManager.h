#pragma once

#include <d3d12.h>

#include <cassert>

#include "DX12.h"
#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/memory/cpu/stringHashMap.h"
#include "SirEngine/rootSignatureManager.h"

namespace SirEngine::graphics {
struct MaterialMetadata;
}

namespace SirEngine {
namespace dx12 {

class Dx12RootSignatureManager final : public RootSignatureManager {
 public:
  Dx12RootSignatureManager()
      : m_rootRegister(RESERVE_SIZE), m_rsPool(RESERVE_SIZE){};
  Dx12RootSignatureManager(const Dx12RootSignatureManager &) = delete;
  Dx12RootSignatureManager &operator=(const Dx12RootSignatureManager &) =
      delete;
  ~Dx12RootSignatureManager() override = default;
  void initialize() override {}
  void cleanup() override;

  RSHandle loadSignatureFromMeta(const char *name,
                                 graphics::MaterialMetadata *metadata);

  inline ID3D12RootSignature *getRootSignatureFromName(const char *name) const {
    const RSHandle handle = getHandleFromName(name);
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const RSData &data = m_rsPool.getConstRef(index);
    return data.rs;
  }
  inline ID3D12RootSignature *getRootSignatureFromHandle(
      const RSHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const RSData &data = m_rsPool.getConstRef(index);
    return data.rs;
  }
  void bindGraphicsRS(const RSHandle handle) const override {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const RSData &data = m_rsPool.getConstRef(index);
    auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
    ID3D12GraphicsCommandList2 *commandList = currentFc->commandList;
    commandList->SetGraphicsRootSignature(data.rs);
  }

  void bindComputeRS(const RSHandle &rsHandle) const {
    assertMagicNumber(rsHandle);
    const uint32_t index = getIndexFromHandle(rsHandle);
    const RSData &data = m_rsPool.getConstRef(index);
    auto *currentFc = &dx12::CURRENT_FRAME_RESOURCE->fc;
    ID3D12GraphicsCommandList2 *commandList = currentFc->commandList;
    commandList->SetComputeRootSignature(data.rs);
  }

  RSHandle getHandleFromName(const char *name) const {
    bool result = m_rootRegister.containsKey(name);
    if (!result) {
      SE_CORE_ERROR("Could not find resquested RS {0}", name);
      return {};
    }
    RSHandle value;
    m_rootRegister.get(name, value);
    return value;
  }

  int getBindingSlot(const RSHandle handle, const uint32_t space) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const RSData &data = m_rsPool.getConstRef(index);
    return data.bindingSlots[space];
  }

 private:
  inline void assertMagicNumber(const RSHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_rsPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for root signature");
  }

 private:
  struct RSData {
    ID3D12RootSignature *rs;
    uint32_t magicNumber : 16;
    uint32_t descriptorCount : 15;
    uint32_t isFlatRoot : 1;
    int16_t bindingSlots[4] = {-1, -1, -1, -1};
  };

  HashMap<const char *, RSHandle, hashString32> m_rootRegister;
  SparseMemoryPool<RSData> m_rsPool;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
};
}  // namespace dx12
}  // namespace SirEngine
