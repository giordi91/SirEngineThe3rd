#include "platform/windows/graphics/dx12/shaderManager.h"
#include "platform/windows/graphics/dx12/shaderCompiler.h"

#include "SirEngine/fileUtils.h"
#include <iostream>

namespace temp{
namespace rendering {

void ShaderManager::cleanup() {
  // we need to de-allocate everything
  for (auto s : m_stringToShader) {
    s.second->Release();
  }
  m_stringToShader.clear();
  //Singleton<ShaderManager>::cleanup();
}

void ShaderManager::loadShaderFile(const char *path) {

  auto exp_path = std::experimental::filesystem::path(path);
  std::string name =exp_path.stem().string();
  if (m_stringToShader.find(name) == m_stringToShader.end()) {
    ID3DBlob *blob = loadCompiledShader(path);
    m_stringToShader[name] = blob;
  }
}

void ShaderManager::loadShadersInFolder(const char *directory) {
  std::vector<std::string> paths;
  listFilesInFolder(directory, paths, "cso");

  for (const auto &p : paths) {
    loadShaderFile(p.c_str());
  }
}

} // namespace rendering
} // namespace dx12
