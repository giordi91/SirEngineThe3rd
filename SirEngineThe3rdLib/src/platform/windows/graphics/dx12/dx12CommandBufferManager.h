#pragma once

#include <assert.h>
#include <d3d12.h>

#include "SirEngine/graphics/commandBufferManager.h"
#include "SirEngine/memory/cpu/SparseMemoryPool.h"

namespace SirEngine::dx12 {

class Dx12CommandBufferManager final : public CommandBufferManager {
  static constexpr uint32_t RESERVE_SIZE = 50;

 public:
  struct Dx12CommandBufferData {
    ID3D12CommandAllocator *commandAllocator = nullptr;
#if DXR_ENABLED
    ID3D12GraphicsCommandList4 *commandList = nullptr;
#else
    ID3D12GraphicsCommandList2 *commandList = nullptr;
#endif
    COMMAND_BUFFER_ALLOCATION_FLAGS flags;
    uint32_t version;
    bool isListOpen = false;
  };

 public:
  Dx12CommandBufferManager()
      : CommandBufferManager(), m_bufferPool(RESERVE_SIZE) {}
  ~Dx12CommandBufferManager() override = default;

  Dx12CommandBufferManager(const Dx12CommandBufferManager &) = delete;
  Dx12CommandBufferManager &operator=(const Dx12CommandBufferManager &) =
      delete;

  void initialize() override {}
  void cleanup() override {}

  CommandBufferHandle createBuffer(COMMAND_BUFFER_ALLOCATION_FLAGS flags,
                                   const char *name = nullptr) override;
  void executeBuffer(CommandBufferHandle handle) override;
  void resetBufferHandle(CommandBufferHandle handle) override;
  void executeFlushAndReset(CommandBufferHandle handle) override;
  void freeBuffer(CommandBufferHandle handle) override;

  // We allow to get the data directly, mostly because once we are in Vulkan
  // land might be beneficial to extract the buffer once and do operation on it
  // rather than wrap every single function ,should be faster and less code foot
  // print
  [[nodiscard]] const Dx12CommandBufferData &getData(
      const CommandBufferHandle handle) const {
    assertVersion(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    return m_bufferPool.getConstRef(idx);
  }

 private:
  inline void assertVersion(const CommandBufferHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    auto const &ref = m_bufferPool.getConstRef(idx);
    assert(ref.version == magic && "invalid magic handle for command buffer");
  }

 private:
  SparseMemoryPool<Dx12CommandBufferData> m_bufferPool;
  uint32_t m_versionCounter = 1;
};
}  // namespace SirEngine::dx12
