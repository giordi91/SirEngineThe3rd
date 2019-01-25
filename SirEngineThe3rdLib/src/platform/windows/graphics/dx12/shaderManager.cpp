#include "platform/windows/graphics/dx12/shaderManager.h"

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include <d3dcompiler.h>

namespace SirEngine{
namespace dx12{

void ShaderManager::cleanup() {
  // we need to de-allocate everything
  for (auto s : m_stringToShader) {
    s.second->Release();
  }
  m_stringToShader.clear();
}

ID3DBlob *loadCompiledShader(const std::string &filename) {
  std::ifstream fin(filename, std::ios::binary);
  assert(!fin.fail());
  fin.seekg(0, std::ios_base::end);
  std::ifstream::pos_type size = (int)fin.tellg();
  fin.seekg(0, std::ios_base::beg);

  ID3DBlob *blob;
  HRESULT hr = D3DCreateBlob(size, &blob);
  assert(SUCCEEDED(hr));

  fin.read(static_cast<char *>(blob->GetBufferPointer()), size);
  fin.close();
  return blob;
}

void ShaderManager::loadShaderFile(const char *path) {

  auto exp_path = std::experimental::filesystem::path(path);
  std::string name = exp_path.stem().string();
  if (m_stringToShader.find(name) == m_stringToShader.end()) {
    ID3DBlob *blob = loadCompiledShader(path);
	int size = blob->GetBufferSize();
	auto* ptr = blob->GetBufferPointer();
    m_stringToShader[name] = blob;
  }
}
void ShaderManager::loadShaderBinaryFile(const char *path) {

  auto exp_path = std::experimental::filesystem::path(path);
  std::string name = exp_path.stem().string();
  if (m_stringToShader.find(name) == m_stringToShader.end()) {
    // ID3DBlob *blob = loadCompiledShader(path);

    std::vector<char> data;
    readAllBytes(path, data);

    const BinaryFileHeader *h = getHeader(data.data());
    auto mapper = getMapperData<ShaderMapperData>(data.data());
	void* shaderPointer = data.data() + sizeof(BinaryFileHeader);
	ID3DBlob* blob;
	HRESULT hr = D3DCreateBlob(mapper->shaderSizeInBtye, &blob);
	memcpy(blob->GetBufferPointer(), shaderPointer, blob->GetBufferSize());
    m_stringToShader[name] = blob;
  }
}

void ShaderManager::loadShadersInFolder(const char *directory) {
  std::vector<std::string> paths;

  //lets look first for shader from our resource compiler
  listFilesInFolder(directory, paths, "shader");
  for (const auto &p : paths) {
    loadShaderBinaryFile(p.c_str());
  }

  //lets look for normally compiled shaders from visual studio
  paths.clear();
  listFilesInFolder(directory, paths, "cso");
  for (const auto &p : paths) {
    loadShaderFile(p.c_str());
  }
}

} // namespace rendering
} // namespace temp
