#pragma once
#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

#include "DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include "nlohmann/json_fwd.hpp"

namespace temp{
namespace rendering {
class RootSignatureManager;
class ShaderManager;
class ShadersLayoutRegistry;

enum class PSOType { DXR = 0, RASTER, COMPUTE, INVALID };

class PSOManager 
{

public:
  virtual ~PSOManager() = default;
  void init(ID3D12Device4 *device, ShadersLayoutRegistry*,RootSignatureManager*,ShaderManager* );
  void cleanup();
  void loadPSOInFolder(const char *directory);

  // debugging function to be able to print to console the composition of a
  // state object
  void printStateObjectDesc(const D3D12_STATE_OBJECT_DESC *desc);
  inline ID3D12StateObject *getDXRPSOByName(const std::string &name) {

    auto found = m_psoDXRRegister.find(name);
    if (found != m_psoDXRRegister.end()) {
      return found->second;
    }
	std::string outString = "could not find PSO in the registry ";
	outString += name;
    assert(0 && name.c_str());
    return nullptr;
  }
  inline ID3D12PipelineState *getComputePSOByName(const std::string &name) {
    auto found = m_psoRegister.find(name);
    if (found != m_psoRegister.end()) {
      return found->second;
    }
    assert(0 && "could not find Compute PSO in the registry");
    return nullptr;
  }

private:
  void loadPSOFile(const char *path);
  //void processDXRPSO(nlohmann::json &jobj, const std::string &path);
  void processComputePSO(nlohmann::json &jobj, const std::string &path);
  void processRasterPSO(nlohmann::json &jobj, const std::string &path);
  //void processHitGrops(nlohmann::json &jobj, CD3DX12_STATE_OBJECT_DESC &pipe);
  //void processPayload(nlohmann::json &jobj, CD3DX12_STATE_OBJECT_DESC &pipe);
  //void processLocalRootSignatures(nlohmann::json &jobj,
  //                                CD3DX12_STATE_OBJECT_DESC &pipe);
  void processGlobalRootSignature(nlohmann::json &jobj,
                                  CD3DX12_STATE_OBJECT_DESC &pipe);
  void processPipelineConfig(nlohmann::json &jobj,
                             CD3DX12_STATE_OBJECT_DESC &pipe);

private:
  ID3D12Device4 *m_dxrDevice = nullptr;
  std::unordered_map<std::string, ID3D12StateObject *> m_psoDXRRegister;
  std::unordered_map<std::string, ID3D12PipelineState *> m_psoRegister;

  ShadersLayoutRegistry *layoutManger = nullptr;
  RootSignatureManager *rs_manager = nullptr;
  ShaderManager *shaderManager = nullptr;
};
} // namespace rendering
} // namespace dx12
