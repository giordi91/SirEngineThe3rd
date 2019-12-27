#pragma once

namespace SirEngine::dx12 {
class RootSignatureManager;
class ShaderManager;
class ShadersLayoutRegistry;

// TODO make it not copyable assignable
class PSOManager final {

public:
  /*
  PSOManager(){};
  virtual ~PSOManager() = default;
  PSOManager(const PSOManager &) = delete;
  PSOManager &operator=(const PSOManager &) = delete;

  void init(D3D12DeviceType *device, SirEngine::dx12::ShadersLayoutRegistry *,
            SirEngine::dx12::RootSignatureManager *,
            SirEngine::dx12::ShaderManager *);
  void cleanup() = 0;
  void loadRawPSOInFolder(const char *directory);
  void loadCachedPSOInFolder(const char *directory);

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

  void recompilePSOFromShader(const char *shaderName,
                              const char *getOffsetPath);
  inline void bindPSO(const PSOHandle handle,
                      ID3D12GraphicsCommandList2 *commandList) const {

    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    commandList->SetPipelineState(data.pso);
  }

  inline PSOHandle getHandleFromName(const char *name) const {

    // assert(m_psoRegisterHandle.containsKey(name));
    bool r = m_psoRegisterHandle.containsKey(name);
    if (!r) {
      // assert(0);
      SE_CORE_ERROR("Could not find PSO {0}", name);
      return {};
    }
    PSOHandle value{};
    m_psoRegisterHandle.get(name, value);
    return value;
  }

private:
  // PSOCompileResult processComputePSO(nlohmann::json &jobj,
  //                                   const std::string &path);
  // PSOCompileResult processRasterPSO(nlohmann::json &jobj,
  //                                  const std::string &path);

  void processGlobalRootSignature(nlohmann::json &jobj,
                                  CD3DX12_STATE_OBJECT_DESC &pipe) const;
  void processPipelineConfig(nlohmann::json &jobj,
                             CD3DX12_STATE_OBJECT_DESC &pipe) const;
  PSOCompileResult loadCachedPSO(const char *path);

private:
  void updatePSOCache(const char *name, ID3D12PipelineState *pso);
  void insertInPSOCache(const PSOCompileResult &result);

  inline uint32_t getIndexFromHandle(const PSOHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(const PSOHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }
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
  */
};
} // namespace SirEngine::dx12
