#pragma once
#include <cassert>
#include <string>
#include <unordered_map>

// forward declares
struct ID3D12RootSignature;

namespace SirEngine{
namespace dx12{

enum class ROOT_FILE_TYPE { RASTER = 0, COMPUTE = 1, DXR = 2, NULL_TYPE };

class RootSignatureManager final
{

public:
  RootSignatureManager() = default;
  RootSignatureManager(const RootSignatureManager &) = delete;
  RootSignatureManager &operator=(const RootSignatureManager &) = delete;
  ~RootSignatureManager() = default;
  void cleanup();
  void loadSingaturesInFolder(const char *directory);
  void loadSignatureBinaryFile(const char *directory);
  inline ID3D12RootSignature *getRootSignatureFromName(const char *name) const {
    auto found = m_rootRegister.find(name);
    if (found != m_rootRegister.end()) {
      return found->second;
    }
    assert(0 && "could not find requested root signature in register");
	return nullptr;
  }

private:
  std::unordered_map<std::string, ID3D12RootSignature *> m_rootRegister;
};
} // namespace rendering
} // namespace temp
