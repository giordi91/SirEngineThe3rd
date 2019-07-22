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
  for (const auto& it : m_rootRegister) {
	  getRootSignatureFromHandle(it.second)->Release();
  }
  m_rootRegister.clear();
}

void RootSignatureManager::loadSignatureBinaryFile(const char *file) {

  // check the file exists and read all the binary data out of the file
  const auto expPath = std::experimental::filesystem::path(file);
  const std::string name = expPath.stem().string();
  if (m_rootRegister.find(name) == m_rootRegister.end()) {

    std::vector<char> data;
    const bool fileOpenRes = readAllBytes(file, data);
    if (!fileOpenRes) {
      return;
    }

    // extract the header and figure out the mapping of the file
    const BinaryFileHeader *h = getHeader(data.data());
    if (!(h->fileType == BinaryFileType::RS)) {
      SE_CORE_ERROR(
          "Root signature manager: cannot load RS: \n {0} \n file type is {1}",
          file,
          getBinaryFileTypeName(static_cast<BinaryFileType>(h->fileType)));
      return;
    }
    const auto mapper = getMapperData<RootSignatureMappedData>(data.data());
    void *rootSignaturePointer = data.data() + sizeof(BinaryFileHeader);
    ID3DBlob *blob;
    const HRESULT hr = D3DCreateBlob(mapper->sizeInByte, &blob);
    assert(SUCCEEDED(hr) && "failed create blob of data for root signature");
    memcpy(blob->GetBufferPointer(), rootSignaturePointer,
           blob->GetBufferSize());

    ID3D12RootSignature *rootSig;
    const HRESULT res = SirEngine::dx12::DEVICE->CreateRootSignature(
        1, blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS(&(rootSig)));
    assert(res == S_OK);
    blob->Release();


	//generate the handle
    uint32_t index;
    RSData &rsdata= m_rsPool.getFreeMemoryData(index);
    rsdata.rs= rootSig;
    const RSHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
    rsdata.magicNumber = MAGIC_NUMBER_COUNTER;
    m_rootRegister[name] = handle;
    ++MAGIC_NUMBER_COUNTER;


  }
}

void RootSignatureManager::loadSignaturesInFolder(const char *directory) {

  std::vector<std::string> paths;
  listFilesInFolder(directory, paths, "root");

  for (const auto &p : paths) {
    loadSignatureBinaryFile(p.c_str());
  }
}

} // namespace dx12
} // namespace SirEngine
