#pragma once

#include "DX12.h"
#include "SirEngine/PSOManager.h"
#include "SirEngine/memory/cpu/resizableVector.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/memory/cpu/stringHashMap.h"
#include "nlohmann/json_fwd.hpp"
#include "platform/windows/graphics/dx12/PSOCompile.h"
#include "platform/windows/graphics/dx12/d3dx12.h"

namespace SirEngine::dx12 {

class Dx12PSOManager final : public PSOManager {
  struct PSOData {
    ID3D12PipelineState *pso;
    uint32_t magicNumber;
    TOPOLOGY_TYPE topology;
    ID3D12RootSignature *root;
    PSOType type;
  };

 public:
  Dx12PSOManager()
      : PSOManager(),
        m_psoDXRRegister(RESERVE_SIZE),
        m_psoRegister(RESERVE_SIZE),
        m_psoRegisterHandle(RESERVE_SIZE),
        m_shaderToPSOFile(RESERVE_SIZE),
        m_psoPool(RESERVE_SIZE){};
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
                              const char *getOffsetPath) override;
  inline void bindPSO(const PSOHandle handle,
                      ID3D12GraphicsCommandList2 *commandList,
                      const bool bindRoot = false) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    commandList->SetPipelineState(data.pso);
    if (bindRoot) {
      if (data.type == PSOType::RASTER) {
        commandList->SetGraphicsRootSignature(data.root);
      } else {
        commandList->SetComputeRootSignature(data.root);
      }
    }
    // TODO move this to the bind pso
    switch (data.topology) {
      case (TOPOLOGY_TYPE::TRIANGLE): {
        commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        break;
      }
      case TOPOLOGY_TYPE::UNDEFINED: {
        assert(0 && "trying to bind undefined topology");
        return;
      }
      case TOPOLOGY_TYPE::LINE: {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
        break;
      }
      case TOPOLOGY_TYPE::LINE_STRIP: {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
        break;
      }
      case TOPOLOGY_TYPE::TRIANGLE_STRIP: {
        commandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        break;
      }
      default:;
    }
  }
  void bindPSO(const PSOHandle handle) const override {
    auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;
    bindPSO(handle, commandList, false);
  }

  PSOHandle getHandleFromName(const char *name) const override;
  TOPOLOGY_TYPE getTopology(const PSOHandle psoHandle) const;

 private:
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
  PSOCompileResult loadCachedPSO(const char *path) const;

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
}  // namespace SirEngine::dx12
