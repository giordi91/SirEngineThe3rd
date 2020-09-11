#pragma once

#include <d3d12.h>

#include <cassert>

#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/memory/cpu/stringHashMap.h"
#include "SirEngine/rootSignatureManager.h"

namespace SirEngine {
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
  ~Dx12RootSignatureManager() = default;
  void initialize() override {}
  void cleanup() override;
  void loadSignaturesInFolder(const char *directory) override;
  void loadSignatureBinaryFile(const char *file) override;

  RSHandle loadSignatureFromMeta(const char* name, MaterialMetadata* metadata);

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

  inline void bindGraphicsRS(const RSHandle handle,
                             ID3D12GraphicsCommandList2 *commandList) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const RSData &data = m_rsPool.getConstRef(index);
    commandList->SetGraphicsRootSignature(data.rs);
  }
  void bindComputeRS(const RSHandle &rsHandle,
                     ID3D12GraphicsCommandList2 *commandList) const {
    assertMagicNumber(rsHandle);
    const uint32_t index = getIndexFromHandle(rsHandle);
    const RSData &data = m_rsPool.getConstRef(index);
    commandList->SetComputeRootSignature(data.rs);
  };
  ;
  inline bool isFlatRoot(const RSHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const RSData &data = m_rsPool.getConstRef(index);
    return data.isFlatRoot > 0;
  }
  inline uint32_t getDescriptorCount(const RSHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const RSData &data = m_rsPool.getConstRef(index);
    return data.descriptorCount;
  }

  RSHandle getHandleFromName(const char *name) const {
    bool result = m_rootRegister.containsKey(name);
    if (!result) {
      // TODO change this back to asserting once vulkan port is stable
      SE_CORE_ERROR("Could not find resquested RS {0}", name);
      return {};
    }
    RSHandle value;
    m_rootRegister.get(name, value);
    return value;
  }

  int getBindingSlot(const RSHandle handle, const uint32_t space) const
  {
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
