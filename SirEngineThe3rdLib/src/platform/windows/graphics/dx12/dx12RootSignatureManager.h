#pragma once

#include "SirEngine/rootSignatureManager.h"

#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/memory/stringHashMap.h"

#include <d3d12.h>

#include <cassert>

namespace SirEngine {
namespace dx12 {

class Dx12RootSignatureManager final: public RootSignatureManager {

public:
  Dx12RootSignatureManager()
      :  m_rootRegister(RESERVE_SIZE),
        m_rsPool(RESERVE_SIZE){};
  Dx12RootSignatureManager(const Dx12RootSignatureManager &) = delete;
  Dx12RootSignatureManager &
  operator=(const Dx12RootSignatureManager &) = delete;
  ~Dx12RootSignatureManager() = default;
  void initialize() {};
  void cleanup() ;
  void loadSignaturesInFolder(const char *directory) ;
  void loadSignatureBinaryFile(const char *file) ;

  inline ID3D12RootSignature *getRootSignatureFromName(const char *name) const {

    const RSHandle handle = getHandleFromName(name);
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const RSData &data = m_rsPool.getConstRef(index);
    return data.rs;
  }
  inline ID3D12RootSignature *
  getRootSignatureFromHandle(const RSHandle handle) const {

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

  RSHandle getHandleFromName(const char *name) const {
    assert(m_rootRegister.containsKey(name));
    RSHandle value;
    m_rootRegister.get(name, value);
    return value;
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
    uint32_t magicNumber;
  };

  HashMap<const char *, RSHandle, hashString32> m_rootRegister;
  SparseMemoryPool<RSData> m_rsPool;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
};
} // namespace dx12
} // namespace SirEngine
