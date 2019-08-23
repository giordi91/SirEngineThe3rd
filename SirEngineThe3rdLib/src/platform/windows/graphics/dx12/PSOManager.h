#pragma once
#include "SirEngine/core.h"
#include <cassert>
#include <string>
#include <unordered_map>

#include "DX12.h"
#include "SirEngine/memory/SparseMemoryPool.h"
#include "nlohmann/json_fwd.hpp"
#include "platform/windows/graphics/dx12/d3dx12.h"

namespace SirEngine::dx12 {
class RootSignatureManager;
class ShaderManager;
class ShadersLayoutRegistry;

enum class PSOType { DXR = 0, RASTER, COMPUTE, INVALID };

class SIR_ENGINE_API PSOManager final {

  struct PSOData {
    ID3D12PipelineState *pso;
    uint32_t magicNumber;
  };

public:
  PSOManager() : m_psoPool(RESERVE_SIZE){};
  ~PSOManager() = default;
  void init(D3D12DeviceType *device, SirEngine::dx12::ShadersLayoutRegistry *,
            SirEngine::dx12::RootSignatureManager *,
            SirEngine::dx12::ShaderManager *);
  void cleanup();
  void loadPSOInFolder(const char *directory);

  // debugging function to be able to print to console the composition of a
  // state object
  static void printStateObjectDesc(const D3D12_STATE_OBJECT_DESC *desc);
  inline ID3D12PipelineState *
  getComputePSOByName(const std::string &name) const {
    const PSOHandle handle = getHandleFromName(name);
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return data.pso;
  }

  void recompilePSOFromShader(const char *shaderName,
                              const char *getOffsetPath);
  inline void bindPSO(const PSOHandle handle,
                      ID3D12GraphicsCommandList2 *commandList) const {

    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    commandList->SetPipelineState(data.pso);
  }

  inline PSOHandle getHandleFromName(const std::string &name) const {
    const auto found = m_psoRegisterHandle.find(name);
    if (found != m_psoRegisterHandle.end()) {
      return found->second;
    }
    assert(0 && "could not find PSO from name");
    return PSOHandle{};
  }

private:
  void loadPSOFile(const char *path, bool reload = false);
  void processComputePSO(nlohmann::json &jobj, const std::string &path,
                         bool reload);
  void processRasterPSO(nlohmann::json &jobj, const std::string &path,
                        bool reload);
  void processGlobalRootSignature(nlohmann::json &jobj,
                                  CD3DX12_STATE_OBJECT_DESC &pipe) const;
  void processPipelineConfig(nlohmann::json &jobj,
                             CD3DX12_STATE_OBJECT_DESC &pipe) const;

private:
  inline uint32_t getIndexFromHandle(const PSOHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(const PSOHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }
  inline void assertMagicNumber(const PSOHandle handle) const {
    uint32_t magic = getMagicFromHandle(handle);
    uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_psoPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for constant buffer");
  }

private:
  D3D12DeviceType *m_dxrDevice = nullptr;
  std::unordered_map<std::string, ID3D12StateObject *> m_psoDXRRegister;
  std::unordered_map<std::string, ID3D12PipelineState *> m_psoRegister;
  std::unordered_map<std::string, PSOHandle> m_psoRegisterHandle;

  // TODO temporary horrible nested data struct will need to think about thi
  std::unordered_map<std::string, std::vector<std::string>> m_shaderToPSOFile;

  ShadersLayoutRegistry *layoutManger = nullptr;
  RootSignatureManager *rs_manager = nullptr;
  ShaderManager *shaderManager = nullptr;

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
