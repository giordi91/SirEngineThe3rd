#pragma once
#include "DX12.h"
#include "SirEngine/core.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/SparseMemoryPool.h"
#include <cassert>
#include <string>
#include <unordered_map>

namespace SirEngine {
namespace dx12 {

enum class ROOT_FILE_TYPE { RASTER = 0, COMPUTE = 1, DXR = 2, NULL_TYPE };

class SIR_ENGINE_API RootSignatureManager final {

public:
  RootSignatureManager() : m_rsPool(RESERVE_SIZE){};
  RootSignatureManager(const RootSignatureManager &) = delete;
  RootSignatureManager &operator=(const RootSignatureManager &) = delete;
  ~RootSignatureManager() = default;
  void cleanup();
  void loadSignaturesInFolder(const char *directory);
  void loadSignatureBinaryFile(const char *file);

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

  inline RSHandle getHandleFromName(const std::string &name) const {
    const auto found = m_rootRegister.find(name);
    if (found != m_rootRegister.end()) {
      return found->second;
    }
    assert(0 && "could not find RS from name");
    return RSHandle{};
  }

private:
  inline uint32_t getIndexFromHandle(const RSHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(const RSHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }
  inline void assertMagicNumber(const RSHandle handle) const {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_rsPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for constant buffer");
  }

private:
  struct RSData {
    ID3D12RootSignature *rs;
    uint32_t magicNumber;
  };

  std::unordered_map<std::string, RSHandle> m_rootRegister;
  // handles
  SparseMemoryPool<RSData> m_rsPool;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
};
} // namespace dx12
} // namespace SirEngine
