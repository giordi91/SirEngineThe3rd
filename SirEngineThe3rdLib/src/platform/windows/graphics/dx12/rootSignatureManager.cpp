#include "platform/windows/graphics/dx12/rootSignatureManager.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/dx12/DX12.h"
#include "platform/windows/graphics/dx12/d3dx12.h"
#include <d3d12.h>
#include <d3dcompiler.h>

namespace SirEngine {
namespace dx12 {

void RootSignatureManager::cleanup() {
  // cleanup the allocated root signatures
  for (auto it : m_rootRegister) {
    it.second->Release();
  }
  m_rootRegister.clear();
}

void RootSignatureManager::loadSignatureBinaryFile(const char *directory) {
  auto exp_path = std::experimental::filesystem::path(directory);
  std::string name = exp_path.stem().string();
  if (m_rootRegister.find(name) == m_rootRegister.end()) {

    std::vector<char> data;
    bool fileOpenRes= readAllBytes(directory, data);
    if (!fileOpenRes) {
      return;
    }

    const BinaryFileHeader *h = getHeader(data.data());
    if (!(h->fileType == BinaryFileType::RS)) {
      SE_CORE_ERROR(
          "Root signature manager: cannot load RS: \n {0} \n file type is {1}",
          directory,
          getBinaryFileTypeName(static_cast<BinaryFileType>(h->fileType)));
      return;
    }
    auto mapper = getMapperData<RootSignatureMappedData>(data.data());
    void *shaderPointer = data.data() + sizeof(BinaryFileHeader);
    ID3DBlob *blob;
    HRESULT hr = D3DCreateBlob(mapper->sizeInByte, &blob);
	assert(SUCCEEDED(hr) && "failed create blob of data for root signature");
    memcpy(blob->GetBufferPointer(), shaderPointer, blob->GetBufferSize());

    ID3D12RootSignature *rootSig;
    HRESULT res = SirEngine::dx12::DEVICE->CreateRootSignature(
        1, blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS(&(rootSig)));
    assert(res == S_OK);
    blob->Release();
    m_rootRegister[name] = rootSig;
  }
}

void RootSignatureManager::loadSingaturesInFolder(const char *directory) {

  std::vector<std::string> paths;
  // listFilesInFolder(directory, paths, "json");
  listFilesInFolder(directory, paths, "root");

  for (const auto &p : paths) {
    // loadSignatureFile(p.c_str());
    loadSignatureBinaryFile(p.c_str());
  }
}

} // namespace dx12
} // namespace SirEngine
