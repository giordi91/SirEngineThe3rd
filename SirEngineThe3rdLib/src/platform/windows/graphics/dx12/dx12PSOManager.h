#pragma once

#include "DX12.h"
#include "SirEngine/PSOManager.h"
#include "SirEngine/memory/resizableVector.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/memory/stringHashMap.h"
#include "platform/windows/graphics/dx12/PSOCompile.h"
#include "platform/windows/graphics/dx12/d3dx12.h"

#include "nlohmann/json_fwd.hpp"

namespace SirEngine::dx12 {

class Dx12PSOManager final : public PSOManager {

  struct PSOData {
    ID3D12PipelineState *pso;
    uint32_t magicNumber;
  };

public:
  Dx12PSOManager()
      : PSOManager(), m_psoDXRRegister(RESERVE_SIZE),
        m_psoRegister(RESERVE_SIZE), m_psoRegisterHandle(RESERVE_SIZE),
        m_shaderToPSOFile(RESERVE_SIZE), m_psoPool(RESERVE_SIZE){};
  virtual ~Dx12PSOManager() = default;
  Dx12PSOManager(const Dx12PSOManager &) = delete;
  Dx12PSOManager &operator=(const Dx12PSOManager &) = delete;
  Dx12PSOManager(Dx12PSOManager &&) = delete;
  Dx12PSOManager &operator=(Dx12PSOManager &&) = delete;

  void initialize() override{};
  void cleanup() override;
  void loadRawPSOInFolder(const char *directory) override;
  void loadCachedPSOInFolder(const char *directory) override;

  void recompilePSOFromShader(const char *shaderName,
                              const char *getOffsetPath);
  inline void bindPSO(const PSOHandle handle,
                      ID3D12GraphicsCommandList2 *commandList) const {

    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    commandList->SetPipelineState(data.pso);
  }

  PSOHandle getHandleFromName(const char *name) const override;

private:
  // PSOCompileResult processComputePSO(nlohmann::json &jobj,
  //                                   const std::string &path);
  // PSOCompileResult processRasterPSO(nlohmann::json &jobj,
  //                                  const std::string &path);

  // debugging function to be able to print to console the composition of a
  // state object
  static void printStateObjectDesc(const D3D12_STATE_OBJECT_DESC *desc);
  inline ID3D12PipelineState *getComputePSOByName(const char *name) const {
    const PSOHandle handle = getHandleFromName(name);
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return data.pso;
  }

  void processGlobalRootSignature(nlohmann::json &jobj,
                                  CD3DX12_STATE_OBJECT_DESC &pipe) const;
  void processPipelineConfig(nlohmann::json &jobj,
                             CD3DX12_STATE_OBJECT_DESC &pipe) const;
  PSOCompileResult loadCachedPSO(const char *path);

  void updatePSOCache(const char *name, ID3D12PipelineState *pso);
  void insertInPSOCache(const PSOCompileResult &result);

  inline void assertMagicNumber(const PSOHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_psoPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for constant buffer");
  }

private:
  D3D12DeviceType *m_dxrDevice = nullptr;

  HashMap<const char *, ID3D12StateObject *, hashString32> m_psoDXRRegister;
  HashMap<const char *, ID3D12PipelineState *, hashString32> m_psoRegister;
  HashMap<const char *, PSOHandle, hashString32> m_psoRegisterHandle;

  HashMap<const char *, ResizableVector<const char *> *, hashString32>
      m_shaderToPSOFile;

  // this is only used for the hot recompilation
  std::string compileLog;

  // handles
  SparseMemoryPool<PSOData> m_psoPool;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
};
} // namespace SirEngine::dx12