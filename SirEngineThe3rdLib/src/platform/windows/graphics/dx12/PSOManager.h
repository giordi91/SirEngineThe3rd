#pragma once
#include "SirEngine/core.h"
#include <cassert>
#include <string>
#include <unordered_map>

#include "DX12.h"
#include "nlohmann/json_fwd.hpp"
#include "platform/windows/graphics/dx12/d3dx12.h"

namespace SirEngine {
namespace dx12 {
class RootSignatureManager;
class ShaderManager;
class ShadersLayoutRegistry;

enum class PSOType { DXR = 0, RASTER, COMPUTE, INVALID };

class SIR_ENGINE_API PSOManager final {

public:
  ~PSOManager() = default;
  void init(D3D12DeviceType *device, SirEngine::dx12::ShadersLayoutRegistry *,
            SirEngine::dx12::RootSignatureManager *,
            SirEngine::dx12::ShaderManager *);
  void cleanup();
  void loadPSOInFolder(const char *directory);

  // debugging function to be able to print to console the composition of a
  // state object
  static void printStateObjectDesc(const D3D12_STATE_OBJECT_DESC *desc);
  inline ID3D12PipelineState *getComputePSOByName(const std::string &name) {
    auto found = m_psoRegister.find(name);
    if (found != m_psoRegister.end()) {
      return found->second;
    }
    assert(0 && "could not find Compute PSO in the registry");
    return nullptr;
  }

  void recompileShader(const char *shaderName);

private:
  void loadPSOFile(const char *path);
  void processComputePSO(nlohmann::json &jobj, const std::string &path);
  void processRasterPSO(nlohmann::json &jobj, const std::string &path);
  void processGlobalRootSignature(nlohmann::json &jobj,
                                  CD3DX12_STATE_OBJECT_DESC &pipe) const;
  void processPipelineConfig(nlohmann::json &jobj,
                             CD3DX12_STATE_OBJECT_DESC &pipe) const;

private:
  D3D12DeviceType *m_dxrDevice = nullptr;
  std::unordered_map<std::string, ID3D12StateObject *> m_psoDXRRegister;
  std::unordered_map<std::string, ID3D12PipelineState *> m_psoRegister;

  //TODO temporary horrible nested data struct will need to thinkk about thi
  std::unordered_map<std::string, std::vector<std::string>> m_shaderToPSOFile;

  SirEngine::dx12::ShadersLayoutRegistry *layoutManger = nullptr;
  SirEngine::dx12::RootSignatureManager *rs_manager = nullptr;
  SirEngine::dx12::ShaderManager *shaderManager = nullptr;

  //this is only used for the hot recompilation
  std::string compileLog;

};
} // namespace dx12
} // namespace SirEngine
