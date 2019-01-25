#pragma once
#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

#include "nlohmann/json_fwd.hpp"
// forward declares
struct CD3DX12_ROOT_PARAMETER;
struct CD3DX12_DESCRIPTOR_RANGE;
struct ID3D12RootSignature;
struct D3D12_ROOT_SIGNATURE_DESC;
struct ID3D12Device;

namespace temp {
namespace rendering {

enum class ROOT_FILE_TYPE { RASTER = 0, COMPUTE = 1, DXR = 2, NULL_TYPE };

class RootSignatureManager {

  enum class ROOT_TYPES {
    CONSTANT,
    DESCRIPTOR_TABLE,
    UAV,
    SRV,
    CBV,
    NULL_TYPE
  };

public:
  RootSignatureManager() = default;
  RootSignatureManager(const RootSignatureManager &) = delete;
  RootSignatureManager &operator=(const RootSignatureManager &) = delete;
  virtual ~RootSignatureManager() = default;
  void init(ID3D12Device *device);
  void cleanup();
  void loadSingaturesInFolder(const char *directory);
  void loadSignatureBinaryFile(const char* directory);
  void clear();
  inline ID3D12RootSignature *getRootSignatureFromName(const char *name) {
    auto found = m_rootRegister.find(name);
    if (found != m_rootRegister.end()) {
      return found->second;
    }
    assert(0 && "could not find requested root signature in register");
    return nullptr;
  }
  void
  SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC &desc,
                                            ID3D12RootSignature **rootSig);

private:
  void loadSignatureFile(const char *path);
  static ROOT_TYPES getTypeEnum(const std::string &type);
  void processDescriptorTable(nlohmann::json &jobj,
                              CD3DX12_ROOT_PARAMETER &param);
  void processSRV(nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param);
  void processCBV(nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param);
  void processConstant(nlohmann::json &jobj, CD3DX12_ROOT_PARAMETER &param);
  void initDescriptorAsUAV(nlohmann::json &jobj,
                           CD3DX12_DESCRIPTOR_RANGE &descriptor);
  void initDescriptorAsSRV(nlohmann::json &jobj,
                           CD3DX12_DESCRIPTOR_RANGE &descriptor);
  void initDescriptorAsConstant(nlohmann::json &jobj,
                                CD3DX12_DESCRIPTOR_RANGE &descriptor);
  void createNullRootSignature();

private:
  ID3D12Device *m_device;
  std::unordered_map<std::string, ID3D12RootSignature *> m_rootRegister;

  // hardcoded keys
  static const std::string ROOT_KEY_CONFIG;
  static const std::string ROOT_KEY_NAME;
  static const std::string ROOT_KEY_TYPE;
  static const std::string ROOT_KEY_DATA;
  static const std::string ROOT_KEY_NUM_DESCRIPTOR;
  static const std::string ROOT_KEY_RANGES;
  static const std::string ROOT_KEY_REGISTER;
  static const std::string ROOT_KEY_BASE_REGISTER;
  static const std::string ROOT_KEY_SIZE_IN_32_BIT_VALUES;
  static const std::string ROOT_KEY_VISIBILITY;
  static const std::string ROOT_KEY_FLAGS;

  static const std::string ROOT_KEY_FLAGS_LOCAL;
  static const std::string ROOT_EMPTY;

  static const std::unordered_map<std::string, ROOT_TYPES> m_stringToRootType;
};
} // namespace rendering
} // namespace temp
